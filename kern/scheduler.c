/** @file scheduler.c
 *
 *  @brief TODO

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
#include "process.h"
#include "dbgconf.h"
#include "scheduler.h"
#include "context_switch.h"
#include "reaper.h"

extern void dumpAll();

// Scheduler is special enough:
// It's the ONLY place that one RUNNING thread trying to own another RUNNABLE
// or even RUNNING thread (exclude bootstrap the 1st)
// So, it risks some cases when the thread is shceduled by the other core and
// shut itself down (thread data structure does not exist anymore)
// We must be super careful about this.
void yieldToNext() {
  int currentTID = getLocalCPU()->runningTID;
  if (currentTID == -1) {
    return;
  }

  // Don't have to acquire lock, pickNextRunnableThread will do it!
  tcb* currentThread = findTCB(currentTID);
  tcb* nextThread = pickNextRunnableThread(currentThread);
  if (nextThread == NULL) {
    // No other thread, run myself. If myself is not runnalbe (impossible if
    // we have idle), panic
    if (currentThread->status != THREAD_RUNNING) {
      dumpAll();
      panic("yieldToNext: current TID = -1");
    }
    return;
  }
  lprintf("Scheduling to thread #%d", nextThread->id);
  swtichToThread_Prelocked(nextThread);
}

// NULL if there's no other thread runnable
// If not NULL, LocalLock is held because when the current core is going to
// to another, it's unreasonable to be interrupted.
// Otherwise, this thread may own some other thread, but neither of them are
// progressing
// Caller should release it after swtich
tcb* pickNextRunnableThread(tcb* currentThread) {
  tcb* nextThread = currentThread;
  bool needToReleaseFormer = false;
  while (true) {
    nextThread = roundRobinNextTCBWithEphemeralAccess(
        nextThread, needToReleaseFormer);
    if (nextThread == currentThread) {
      // We've gone through everything, no one else is good to go.
      // No need to release ephemeral access, it will not acquire when the next
      // is itself
      return NULL;
    }
    LocalLockR();
    bool owned = __sync_bool_compare_and_swap(
        &nextThread->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD);
    if (!owned) {
      LocalUnlockR();
      #ifdef SCHEDULER_DECISION_PRINT
        lprintf("thrad #%d is owned by the others, skip", nextThread->id);
      #endif
      // someone else is using this, skip.
      needToReleaseFormer = true;
      continue;
    }
    // successfully own the thread, release ephemeral access
    releaseEphemeralAccess(nextThread);
    if (nextThread->status == THREAD_DEAD && !currentThread->descheduling) {
      // time to reap
      LocalUnlockR();
      #ifdef SCHEDULER_DECISION_PRINT
        lprintf("Reaping thread #%d", nextThread->id);
      #endif
      reapThread(nextThread);
      // nextThread may be cleared, it's not proper to refer to it anymore
      // Go back to where we are
      nextThread = currentThread;
      needToReleaseFormer = false;
      continue;
    }
    if (!THREAD_STATUS_CAN_RUN(nextThread->status)) {
      LocalUnlockR();
      #ifdef SCHEDULER_DECISION_PRINT
        lprintf("thread #%d cannot run, skip", nextThread->id);
      #endif
      // Not runnable. Put it back sir.
      nextThread->owned = THREAD_NOT_OWNED;
      needToReleaseFormer = false;
      continue;
    }

    // OK it's you man!
    return nextThread;
  }
}
