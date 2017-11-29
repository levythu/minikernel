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

// Will return false if there's no corresponding IDT entry
bool appendIntTo(void* _info, hvInt hvi) {
  HyperInfo* info = (HyperInfo*)_info;
  if (!info->idt[hvi.intNum].present) {
    lprintf("Non-presented");
    return false;
  }
  bool succ;
  GlobalLockR(&info->latch);
  // TODO may need to append huahua
  succ = varQueueEnq(&info->delayedInt, hvi);
  lprintf("Queue = %d", info->delayedInt.size);
  GlobalUnlockR(&info->latch);
  return succ;
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
