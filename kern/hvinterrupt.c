#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <hvcall_int.h>
#include <hvcall.h>
#include <x86/asm.h>

#include "int_handler.h"
#include "common_kern.h"
#include "bool.h"
#include "process.h"
#include "cpu.h"
#include "hv.h"
#include "hvlife.h"
#include "source_untrusted.h"
#include "hv_hpcall_internal.h"
#include "mode_switch.h"
#include "hvinterrupt.h"
#include "zeus.h"
#include "hvvm.h"
#include "dbgconf.h"

MAKE_VAR_QUEUE_UTILITY(hvInt);

// Will return false if there's no corresponding IDT entry
bool appendIntTo(void* _info, hvInt hvi) {
  assert(hvi.intNum >= 0 && hvi.intNum <= MAX_SUPPORTED_VIRTUAL_INT);
  HyperInfo* info = (HyperInfo*)_info;
  GlobalLockR(&info->latch);
  IDTEntry localIDT = info->idt[hvi.intNum];
  GlobalUnlockR(&info->latch);
  if (!localIDT.present) {
    return false;
  }
  bool succ;
  GlobalLockR(&info->latch);
  succ = varQueueEnq(&info->delayedInt, hvi);
  GlobalUnlockR(&info->latch);
  return succ;
}

static void elevatePriviledge(HyperInfo* info, tcb* thr) {
  assert(!info->inKernelMode);
  info->inKernelMode = true;
  assert(info->originalPD);
  reActivateOriginalPD(thr);
  // Recompile PD to shadow kernel only memories
  if (!swtichGuestPD(thr)) {
    lprintf("Hypervisor crashes: fail to recompile kernel page directory");
    exitHyperWithStatus(info, thr, GUEST_CRASH_STATUS);
  }
}

// One-way function.
// It will not be reentrant. Since it only happens when switching back to
// guest.
bool applyInt(HyperInfo* info, hvInt hvi,
    uint32_t oldESP, uint32_t oldEFLAGS, uint32_t oldEIP,
    int oedi, int oesi, int oebp, int oebx, int oedx, int oecx, int oeax) {

  assert(hvi.intNum >= 0 && hvi.intNum <= MAX_SUPPORTED_VIRTUAL_INT);
  GlobalLockR(&info->latch);
  IDTEntry localIDT = info->idt[hvi.intNum];
  GlobalUnlockR(&info->latch);
  if (!localIDT.present) {
    return false;
  }

  tcb* currentThread = findTCB(getLocalCPU()->runningTID);

  // TODO: address validation
  uint32_t* stack;
  uint32_t newESP;
  // 1. Push state:
  if (info->inKernelMode) {
    // No stack change
    if (!verifyUserSpaceAddr(oldESP + info->baseAddr - 10 * sizeof(uint32_t),
                             oldESP + info->baseAddr - 1,
                             true)) {
      // Not a proper stack to push. Crash the kernel
      lprintf("Hypervisor crashes: not a proper esp on int delivery");
      exitHyperWithStatus(info, currentThread, GUEST_CRASH_STATUS);
    }
    stack = (uint32_t*)(oldESP + info->baseAddr);
    stack[-1] = oldEFLAGS;
    stack[-2] = GUEST_INTERRUPT_KMODE;
    stack[-3] = oldEIP;
    stack[-4] = hvi.spCode;
    stack[-5] = hvi.cr2;
    newESP = (uint32_t)(&stack[-5]) - info->baseAddr;
  } else {
    // Elevate priviledge
    elevatePriviledge(info, currentThread);
    if (!verifyUserSpaceAddr(
          info->esp0 + info->baseAddr - 10 * sizeof(uint32_t),
          info->esp0 + info->baseAddr - 1,
          true)) {
      // Not a proper stack to push. Crash the kernel
      lprintf("Hypervisor crashes: not a proper esp0 on int delivery");
      exitHyperWithStatus(info, currentThread, GUEST_CRASH_STATUS);
    }
    stack = (uint32_t*)(info->esp0 + info->baseAddr);
    stack[-1] = oldESP;
    stack[-2] = oldEFLAGS;
    stack[-3] = GUEST_INTERRUPT_UMODE;
    stack[-4] = oldEIP;
    stack[-5] = hvi.spCode;
    stack[-6] = hvi.cr2;
    newESP = (uint32_t)(&stack[-6]) - info->baseAddr;
  }

  // 2. Prepare to swtich
  switchToRing3X(newESP, oldEFLAGS, localIDT.eip,
                 oedi, oesi, oebp, oebx, oedx, oecx, oeax,
                 info->cs, info->ds);
  return true;
}

// Called on timer interrupt return when it's from kernel to guest
void applyDelayedInt(HyperInfo* info,
    uint32_t oldESP, uint32_t oldEFLAGS, uint32_t oldEIP,
    int oedi, int oesi, int oebp, int oebx, int oedx, int oecx, int oeax) {
  if (!info->interrupt) return;
  GlobalLockR(&info->latch);
  #ifdef HYPERVISOR_VERBOSE_PRINT
    lprintf("%d", info->delayedInt.size);
  #endif
  if (info->delayedInt.size == 0) {
    GlobalUnlockR(&info->latch);
    return;
  }
  info->interrupt = false;
  hvInt hvi = varQueueDeq(&info->delayedInt);
  GlobalUnlockR(&info->latch);

  applyInt(info, hvi, oldESP, oldEFLAGS, oldEIP,
           oedi, oesi, oebp, oebx, oedx, oecx, oeax);
}

// Will be called after each timer event is handled completely.
// See int_handler.S
void hypervisorTimerHook(const int es, const int ds,
    const int _edi, const int _esi, const int _ebp, // pusha region
    const int _espOnCurrentStack, // pusha region
    const int _ebx, const int _edx, const int _ecx, const int _eax, // pusha
    const int eip, const int cs,  // from-user-mode only
    const int eflags, const int esp,  // from-user-mode only
    const int ss  // from-user-mode only
    ) {
  if (es != SEGSEL_GUEST_DS) {
    // either from normal elf or kernel mode. We are not interested.
    return;
  }
  tcb* thr = findTCB(getLocalCPU()->runningTID);
  assert(thr != NULL);
  assert(thr->process->hyperInfo.isHyper);

  applyDelayedInt(&thr->process->hyperInfo, esp, eflags, eip,
                  _edi, _esi, _ebp, _ebx, _edx, _ecx, _eax);
}

/****************************************************************************/
// Multiplexer-related

void initMultiplexer(intMultiplexer* mper){
  kmutexInit(&mper->mutex);
  for (int i = 0; i < MAX_CONCURRENT_VM; i++) {
    mper->waiter[i] = 0;
  }
}

void broadcastIntTo(intMultiplexer* mper, hvInt hvi){
  kmutexRLock(&mper->mutex);
  for (int i = 0; i < MAX_CONCURRENT_VM; i++) {
    if (mper->waiter[i] == 0) continue;
    appendIntTo((HyperInfo*)mper->waiter[i], hvi);
  }
  kmutexRUnlock(&mper->mutex);
}

void addToWaiter(intMultiplexer* mper, void* info){
  kmutexWLock(&mper->mutex);
  int i;
  for (i = 0; i < MAX_CONCURRENT_VM; i++) {
    if (mper->waiter[i] == 0) break;
  }
  if (i == MAX_CONCURRENT_VM) panic("addToWaiter: concurrent vm exceeded.");
  mper->waiter[i] = info;
  kmutexWUnlock(&mper->mutex);
}

void removeWaiter(intMultiplexer* mper, void* info){
  kmutexWLock(&mper->mutex);
  for (int i = 0; i < MAX_CONCURRENT_VM; i++) {
    if (mper->waiter[i] == info) {
      mper->waiter[i] = 0;
      kmutexWUnlock(&mper->mutex);
      return;
    }
  }
  panic("removeWaiter: no specified info");
}
