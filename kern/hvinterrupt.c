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

// One-way function.
void applyInt(HyperInfo* info, hvInt hvi,
    uint32_t oldESP, uint32_t oldEFLAGS, uint32_t oldEIP) {

  assert(hvi.intNum >= 0 && hvi.intNum <= MAX_SUPPORTED_VIRTUAL_INT);
  GlobalLockR(&info->latch);
  IDTEntry localIDT = info->idt[hvi.intNum];
  GlobalUnlockR(&info->latch);
  if (!localIDT.present) {
    return;
  }

  // TODO: address validation
  // TODO: privilege validation
  uint32_t* stack = (uint32_t*)(oldESP + info->baseAddr);
  // 1. Push state:
  // No stack change
  stack[-1] = oldEFLAGS;
  stack[-2] = GUEST_INTERRUPT_KMODE;
  stack[-3] = oldEIP;
  stack[-4] = 0;
  stack[-5] = 0;
  uint32_t newESP = (uint32_t)(&stack[-5]) - info->baseAddr;

  // 2. Prepare to swtich
  switchToRing3X(newESP, oldEFLAGS, localIDT.eip, 0, 0, 0, 0, 0, 0, 0,
                 info->cs, info->ds);
}

// Called on timer interrupt return when it's from kernel to guest
void applyDelayedInt(HyperInfo* info,
    uint32_t oldESP, uint32_t oldEFLAGS, uint32_t oldEIP) {
  if (!info->interrupt) return;
  GlobalLockR(&info->latch);
  lprintf("%d", info->delayedInt.size);
  if (info->delayedInt.size == 0) {
    GlobalUnlockR(&info->latch);
    return;
  }
  info->interrupt = false;
  hvInt hvi = varQueueDeq(&info->delayedInt);
  GlobalUnlockR(&info->latch);

  applyInt(info, hvi, oldESP, oldEFLAGS, oldEIP);
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

  applyDelayedInt(&thr->process->hyperInfo, esp, eflags, eip);
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
