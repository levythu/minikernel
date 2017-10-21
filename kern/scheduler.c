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

// Scheduler is special enough:
// It's the ONLY place that one RUNNING thread trying to own another RUNNABLE
// or even RUNNING thread (exclude bootstrap the 1st)
// So, it risks some cases when the thread is shceduled by the other core and
// shut itself down (thread data structure does not exist anymore)
// We must be super careful about this.
void yieldToNext() {
  int currentTID = getLocalCPU()->runningTID;
  if (currentTID == -1) return;

  tcb* nextThread = pickNextRunnableThread(findTCB(currentTID));
  if (nextThread == NULL) return;
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
  while (true) {
    nextThread = roundRobinNextTCB(nextThread);
    if (nextThread == currentThread) {
      // We've gone through everything, no one else is good to go.
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
      continue;
    }
    if (!THREAD_STATUS_CAN_RUN(nextThread->status)) {
      LocalUnlockR();
      #ifdef SCHEDULER_DECISION_PRINT
        lprintf("thrad #%d cannot run, skip", nextThread->id);
      #endif
      // Not runnable. Put it back sir.
      nextThread->owned = THREAD_NOT_OWNED;
      continue;
    }

    // OK it's you man!
    return nextThread;
  }
}
