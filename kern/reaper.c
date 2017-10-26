/** @file reaper.c
 *
 *  @brief Death walks among you
 *
 *  TODO
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <stdlib.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>

#include <x86/asm.h>
#include <x86/eflags.h>
#include <x86/cr.h>

#include "common_kern.h"
#include "process.h"
#include "bool.h"
#include "process.h"
#include "vm.h"
#include "pm.h"
#include "loader.h"
#include "context_switch.h"
#include "mode_switch.h"
#include "sysconf.h"

static pcb** followZombieChain(pcb* proc) {
  pcb** ret;
  for (ret = &proc->zombieChain; *ret != NULL; ret = &(*ret)->zombieChain)
    ;
  return ret;
}

static void notifyWaiter(pcb* proc) {
  if (proc->waiterThread) {
    // wake up any waiter, need not to own it
    assert(proc->waiterThread->status == THREAD_BLOCKED);
    proc->waiterThread->status = THREAD_RUNNABLE;
    proc->waiterThread = NULL;
  }
}

// 1. Delegate target's zombie children to init
// 2. Tell the parent process that there's a zombie
// Must work under process's mutex
void turnToZombie(pcb* targetProc) {
  targetProc->status = PROCESS_ZOMBIE;
  if (targetProc->zombieChain) {
    // Target has zombie children, give it to init
    pcb* initPCB = findPCBWithEphemeralAccess(INIT_PID);

    kmutexWLock(&initPCB->mutex);
    pcb** chainEnds = followZombieChain(initPCB);
    *chainEnds = targetProc->zombieChain;
    notifyWaiter(initPCB);
    kmutexWUnlock(&initPCB->mutex);
    releaseEphemeralAccessProcess(initPCB);

    targetProc->zombieChain = NULL;
  }

  // Tell my father!
  pcb* directParentPCB = findPCBWithEphemeralAccess(targetProc->parentPID);
  if (directParentPCB) {
    kmutexWLock(&directParentPCB->mutex);
    if (directParentPCB->status == PROCESS_INITIALIZED) {
      // do nothing, it's correct parent
    } else {
      // shoot, my parent is dead, and is waiting for wait, or reaped, I should
      // use INIT instead
      kmutexWUnlock(&directParentPCB->mutex);
      releaseEphemeralAccessProcess(directParentPCB);
      directParentPCB = findPCBWithEphemeralAccess(INIT_PID);
      kmutexWLock(&directParentPCB->mutex);
    }
  } else {
    // my parent is reaped, use init
    directParentPCB = findPCBWithEphemeralAccess(INIT_PID);
    kmutexWLock(&directParentPCB->mutex);
  }
  // Now directParentPCB is a valid one, and the mutex is acquired
  // We are not afraid of circular lock, since parent relationship is a DAG

  assert(directParentPCB->status == PROCESS_INITIALIZED);
  targetProc->zombieChain = directParentPCB->zombieChain;
  directParentPCB->zombieChain = targetProc;
  notifyWaiter(directParentPCB);
  kmutexWUnlock(&directParentPCB->mutex);
  releaseEphemeralAccessProcess(directParentPCB);

  // We are done!
}

// Must work under the process's mutex, must guarantee there's no thread alive
// anymore
void reapProcess(pcb* targetProc) {
  // TODO free process-local resouces
  // TODO remove the process from pcb list
  turnToZombie(targetProc);
}

// targetThread mustn't be current thread, and current CPU need to own it,
// just like the state before context switch. However, it differs that LocalLock
// does not have to be acquried: reaper is free be interrupted
void reapThread(tcb* targetThread) {
  assert(targetThread->status == THREAD_DEAD);

  // Deregister it self from the process
  pcb* targetProc = targetThread->process;
  kmutexWLock(&targetProc->mutex);
  targetProc->numThread--;
  if (targetProc->numThread == 0) {
    reapProcess(targetProc);
  }
  kmutexWUnlock(&targetProc->mutex);

  // TODO free thread-local resources!
  // TODO remove the thread from tcb list

}
