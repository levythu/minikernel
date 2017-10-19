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

// NULL if there's no other thread runnable
tcb* pickNextRunnableThread(tcb* currentThread) {
  tcb* nextThread = currentThread;
  while (true) {
    nextThread = roundRobinNextTCB(nextThread);
    if (nextThread == currentThread) {
      // We've gone through everything, no one else is good to go.
      return NULL;
    }
    bool owned = __sync_bool_compare_and_swap(
        &nextThread->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD);
    if (!owned) {
      #ifdef SCHEDULER_DECISION_PRINT
        lprintf("thrad #%d is owned by the others, skip", nextThread->id);
      #endif
      // someone else is using this, skip.
      continue;
    }
    if (!THREAD_STATUS_CAN_RUN(nextThread->status)) {
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
