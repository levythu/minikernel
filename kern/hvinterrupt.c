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
#include "hvinterrupt.h"
#include "zeus.h"

MAKE_VAR_QUEUE_UTILITY(hvInt);

int hpc_disable_interrupts(int userEsp, tcb* thr) {
  thr->process->hyperInfo.interrupt = false;
  return 0;
}

int hpc_enable_interrupts(int userEsp, tcb* thr) {
  thr->process->hyperInfo.interrupt = true;
  return 0;
}

int hpc_setidt(int userEsp, tcb* thr) {
  DEFINE_PARAM(int, irqno, 0);
  DEFINE_PARAM(uint32_t, eip, 1);
  DEFINE_PARAM(int, privileged, 2);

  if (irqno < 0 || irqno > MAX_SUPPORTED_VIRTUAL_INT) {
    // TODO crash the guest
    return -1;
  }

  HyperInfo* info = &thr->process->hyperInfo;
  GlobalLockR(&info->latch);
  info->idt[irqno].present = true;
  info->idt[irqno].eip = eip;
  info->idt[irqno].privileged = privileged;
  GlobalUnlockR(&info->latch);
  return 0;
}

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

// Called on timer interrupt return when it's from kernel to guest
void applyDelayedInt(HyperInfo* info,
    uint32_t oldESP, uint32_t oldEFLAGS, uint32_t oldEIP) {
  if (info->interrupt) return;
  GlobalLockR(&info->latch);
  if (info->delayedInt.size == 0) {
    GlobalUnlockR(&info->latch);
    return;
  }
  hvInt hvi = varQueueDeq(&info->delayedInt);
  GlobalUnlockR(&info->latch);

  applyInt(info, hvi, oldESP, oldEFLAGS, oldEIP);
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

// Will be called after each timer event is handled completely.
// See int_handler.S
void hypervisorTimerHook(const ) {

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
