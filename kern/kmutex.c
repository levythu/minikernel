/** @file kmutex.c
 *
 *  @brief kernel blocking mutex
 *
 *  The blocking mutex differs from CrossCPULock in cpu.c in that this one is
 *  blocking, instead of spinlock. It deschedule current thread when waiting for
 *  lock.
 *  Therefore, to use it, some notifications need to be kept in mind:
 *  1. Only use it after process/thread is established and scheduler starts
 *  working
 *  2. NEVER use it when interrupt is off (i.e. inside LocalLock or
 *  CrossCPULock).
 *  3. It cannot protect critical session from interrupt, and any reentry due to
 *  it. For that purpose, use CrossCPULock based on your needs.
 *  4. For other cases, use this, especially when critical session is very large
 *  this enables preemtive.
 *
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>

#include "x86/asm.h"
#include "x86/cr.h"
#include "common_kern.h"
#include "pm.h"
#include "vm.h"
#include "bool.h"
#include "cpu.h"
#include "kmutex.h"
#include "process.h"
#include "scheduler.h"
#include "context_switch.h"

void kmutexInit(kmutex* km) {
  initCrossCPULock(&km->spinMutex);
  km->readerWL = NULL;
  km->writerWL = NULL;
  km->status = 0;
}

void kmutexWLockForce(kmutex* km, kmutexStatus* status,
    kmutexStatus* statusToRestore) {
  assert(*status != KMUTEX_HAVE_RLOCK);
  *statusToRestore = *status;
  // If current WLock is acquired, do nothing
  if (*status == KMUTEX_HAVE_WLOCK) return;
  kmutexWLockRecord(km, status);
}
void kmutexWUnlockForce(kmutex* km, kmutexStatus* status,
    kmutexStatus statusToRestore) {
  if (statusToRestore == KMUTEX_HAVE_WLOCK) return;
  kmutexWUnlockRecord(km, status);
}

void kmutexRLock(kmutex* km){
  kmutexRLockRecord(km, NULL);
}

void kmutexRUnlock(kmutex* km){
  kmutexRUnlockRecord(km, NULL);
}

void kmutexWLock(kmutex* km){
  kmutexWLockRecord(km, NULL);
}

void kmutexWUnlock(kmutex* km){
  kmutexWUnlockRecord(km, NULL);
}


void kmutexRLockRecord(kmutex* km, kmutexStatus* status) {
  while (true) {
    GlobalLockR(&km->spinMutex);
    if (status) assert(*status == KMUTEX_NOT_ACQUIRED);
    if (km->status >= 0) {
      // Got it!
      km->status++;
      if (status) *status = KMUTEX_HAVE_RLOCK;
      GlobalUnlockR(&km->spinMutex);
      return;
    }
    // put myself into read wait list, then deschedule myself
    waiterLinklist wl;
    wl.thread = findTCB(getLocalCPU()->runningTID);
    wl.next = km->readerWL;
    km->readerWL = &wl;
    ((tcb*)wl.thread)->descheduling = true;
    ((tcb*)wl.thread)->status = THREAD_BLOCKED;
    removeFromXLX((tcb*)wl.thread);

    GlobalUnlockR(&km->spinMutex);
    yieldToNext();
  }
}

void kmutexRUnlockRecord(kmutex* km, kmutexStatus* status) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  GlobalLockR(&km->spinMutex);
  if (status) {
    assert(*status == KMUTEX_HAVE_RLOCK);
    *status = KMUTEX_NOT_ACQUIRED;
  }
  km->status--;
  if (km->status > 0 || !km->writerWL) {
    // noone else is waiting, or still some one is reading
    GlobalUnlockR(&km->spinMutex);
    return;
  }
  // make the next one in the wl runnable
  waiterLinklist* wl = km->writerWL;
  km->writerWL = wl->next;
  tcb* local = (tcb*)wl->thread;

  GlobalUnlockR(&km->spinMutex);

  LocalLockR();
  if (local->status != THREAD_BLOCKED) {
    panic("Local (#%d).status should be blocked, but is %d",
        local->id, local->status);
  }
  // Own the target thread for context switch. Since it's blocked, no one
  // should really own it, other cores may be accessing it temprorarily.
  // Should never loop on single core
  while (!__sync_bool_compare_and_swap(
      &local->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD))
    ;
  local->status = THREAD_RUNNABLE;
  addToXLX(local);
  if (!currentThread->descheduling) {
    swtichToThread_Prelocked(local);
  } else {
    local->owned = THREAD_NOT_OWNED;
    LocalUnlockR();
  }
}

// If not available, construct a waiterLinklist on local stack (the thread will
// not terminate, so it's safe.)
void kmutexWLockRecord(kmutex* km, kmutexStatus* status) {
  while (true) {
    GlobalLockR(&km->spinMutex);
    if (status) assert(*status == KMUTEX_NOT_ACQUIRED);
    if (km->status == 0) {
      // Got it!
      km->status = -1;
      if (status) *status = KMUTEX_HAVE_WLOCK;
      GlobalUnlockR(&km->spinMutex);
      return;
    }
    // put myself into write wait list, then deschedule myself
    waiterLinklist wl;
    wl.thread = findTCB(getLocalCPU()->runningTID);
    wl.next = km->writerWL;
    km->writerWL = &wl;
    ((tcb*)wl.thread)->descheduling = true;
    ((tcb*)wl.thread)->status = THREAD_BLOCKED;
    removeFromXLX((tcb*)wl.thread);

    GlobalUnlockR(&km->spinMutex);
    yieldToNext();
  }
}

void kmutexWUnlockRecord(kmutex* km, kmutexStatus* status) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  GlobalLockR(&km->spinMutex);
  if (status) {
    assert(*status == KMUTEX_HAVE_WLOCK);
    *status = KMUTEX_NOT_ACQUIRED;
  }
  km->status = 0;
  if (!km->readerWL && !km->writerWL) {
    // noone else is waiting
    GlobalUnlockR(&km->spinMutex);
    return;
  }
  // We honer reader!
  waiterLinklist** nextWaiterList =
      km->readerWL ? &km->readerWL : &km->writerWL;

  waiterLinklist* wl = *nextWaiterList;
  *nextWaiterList = wl->next;
  tcb* local = (tcb*)wl->thread;

  GlobalUnlockR(&km->spinMutex);

  LocalLockR();
  assert(local->status == THREAD_BLOCKED);
  // Own the target thread for context switch. Since it's blocked, no one
  // should really own it, other cores may be accessing it temprorarily.
  // Should never loop on single core
  while (!__sync_bool_compare_and_swap(
      &local->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD))
    ;
  local->status = THREAD_RUNNABLE;
  addToXLX(local);
  if (!currentThread->descheduling) {
    swtichToThread_Prelocked(local);
  } else {
    local->owned = THREAD_NOT_OWNED;
    LocalUnlockR();
  }
}
