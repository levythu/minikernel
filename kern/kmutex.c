/** @file kmutex.c
 *
 *  @brief TODO
 *
 *  TODO
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

void kmutexInit(kmutex* km) {
  initCrossCPULock(&km->spinMutex);
  km->queue = NULL;
  km->available = true;
}

// If not available, construct a waiterLinklist on local stack (the thread will
// not terminate, so it's safe.)
void kmutexLock(kmutex* km) {
  GlobalLockR(&km->spinMutex);
  if (!km->available) {
    // Got it!
    km->available = false;
    GlobalUnlockR(&km->spinMutex);
    return;
  }
  // put myself into wait list, then deschedule myself
  waiterLinklist wl;
  wl.thread = findTCB(getLocalCPU()->runningTID);
  wl.next = km->queue;
  km->queue = &wl;
  wl.thread->status = THREAD_BLOCKED;

  GlobalUnlockR(&km->spinMutex);
  yieldToNext();
}

void kmutexUnlock(kmutex* km) {
  GlobalLockR(&km->spinMutex);
  if (!km->queue) {
    km->available = true;
    GlobalUnlockR(&km->spinMutex);
    return;
  }
  // make the next one in the wl runnable
  waiterLinklist* wl = km->queue;
  km->queue = wl->next;

  assert(wl->thread->status == THREAD_BLOCKED);
  wl->thread->status = THREAD_RUNNABLE;

  GlobalUnlockR(&km->spinMutex);

  tryYieldTo(wl->thread);
}
