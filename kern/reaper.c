/** @file reaper.c
 *
 *  @brief Death walks among you
 *
 *  Reaper handles the death of a thread and process. It collaborate with zeus
 *  to be the core function module in OS
 *
 *  A process die in this order:
 *  1. The last thread calls suicideThread() to perform voluntary termination
 *  2. The process's #thread reduce to zero, turn to prezombie and notify parent
 *     (turnToPreZombie())
 *  3. The last thread turn into THREAD_DEAD, and descheduling to others
 *  4. scheduler working, reaper work inside scheduler to find dead thread, and
 *     that thread is found.
 *  5. The thread is destroyed, together with kernel stack and tcb (reapThread)
 *  6. The process is turned into real zombie (reapProcess->turnToZombie), its
 *     resources other than pcb are destroyed
 *  7. ... (others running)
 *  8. The waiter (one thread inside parent process) catches the zombie, and
 *     pcb is destroyed
 *  The whole procedure is fully preemptible
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
#include "dbgconf.h"
#include "kernel_stack_protection.h"

static uint32_t freeUserspace_EachPage(int pdIndex, int ptIndex, PTE* ptentry,
    uint32_t _) {
  uint32_t physicalPage = PE_DECODE_ADDR(*ptentry);
  *ptentry &= ~PE_PRESENT(1);
  freeUserMemPage(physicalPage);

  return 0;
}

// Reclaim all the user space memory refered in the page directory.
void freeUserspace(PageDirectory pd) {
  traverseEntryPageDirectory(pd,
                             STRIP_PD_INDEX(USER_MEM_START),
                             STRIP_PD_INDEX(0xffffffff),
                             freeUserspace_EachPage,
                             0);
}

// Go through a zombie chain, get its last next pointer reference, and count
// the length of zombie chain btw
static pcb** followZombieChain(pcb* proc, int* len) {
  pcb** ret;
  if (len) *len = 0;
  for (ret = &proc->zombieChain; *ret != NULL; ret = &(*ret)->zombieChain) {
    if (len) (*len)++;
  }
  return ret;
}

// Must run with proc's mutex
// Notify `num` waiters in given process's waiter threads
static void notifyWaiter(pcb* proc, int num) {
  while (proc->waiter && num > 0) {
    // wake up one waiter, need not to own it
    tcb* wThread = (tcb*)proc->waiter->thread;
    assert(wThread->status == THREAD_BLOCKED);
    addToXLX(wThread);
    wThread->status = THREAD_RUNNABLE;
    proc->waiter = proc->waiter->next;
    num--;
  }
}

// 1. Delegate target's zombie children to init
// 2. Tell the parent process that there's a zombie
// Must work inside targetProc's zombie to avoid race that one child is trying
// to append zombie to target's zombie chain
void turnToPreZombie(pcb* targetProc) {
  KERNEL_STACK_CHECK;
  targetProc->status = PROCESS_PREZOMBIE;
  if (targetProc->zombieChain) {
    // Target has zombie children, give it to init
    pcb* initPCB = findPCBWithEphemeralAccess(INIT_PID);

    kmutexWLock(&initPCB->mutex);
    int zombieCount;
    followZombieChain(targetProc, &zombieCount);
    pcb** chainEnds = followZombieChain(initPCB, NULL);
    *chainEnds = targetProc->zombieChain;

    initPCB->unwaitedChildProc += zombieCount;
    if (zombieCount > 0) {
      #ifdef VERBOSE_PRINT
      lprintf("Handling %d zombie children to INIT.., now it's %d",
          zombieCount, initPCB->unwaitedChildProc);
      #endif
    }
    notifyWaiter(initPCB, zombieCount);
    kmutexWUnlock(&initPCB->mutex);
    releaseEphemeralAccessProcess(initPCB);

    targetProc->zombieChain = NULL;
  }

  // Tell my father!
  pcb* directParentPCB = findPCBWithEphemeralAccess(targetProc->parentPID);
  bool notMyFather = false;
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
      notMyFather = true;
    }
  } else {
    // my parent is reaped, use init
    directParentPCB = findPCBWithEphemeralAccess(INIT_PID);
    notMyFather = true;
    kmutexWLock(&directParentPCB->mutex);
  }
  // Now directParentPCB is a valid one, and the mutex is acquired
  // We are not afraid of circular lock, since parent relationship is a DAG

  assert(directParentPCB->status == PROCESS_INITIALIZED);
  targetProc->zombieChain = directParentPCB->zombieChain;
  directParentPCB->zombieChain = targetProc;
  if (notMyFather) {
    // this is not the child of INIT, so we need to add unwaitedChildProc
    directParentPCB->unwaitedChildProc += 1;
    #ifdef VERBOSE_PRINT
    lprintf("I'm orphan... contacting INIT, now it's %d",
        directParentPCB->unwaitedChildProc);
    #endif
  }
  notifyWaiter(directParentPCB, 1);
  kmutexWUnlock(&directParentPCB->mutex);
  releaseEphemeralAccessProcess(directParentPCB);

  // We are done!
}

// Run inside reaper.
// So we don't want to acquire any kmutex: the target proc is prezombie and
// won't be touched by anyone.
void turnToZombie(pcb* targetProc) {
  KERNEL_STACK_CHECK;
  assert(targetProc->status == PROCESS_PREZOMBIE);
  GlobalLockR(&targetProc->prezombieWatcherLock);
  targetProc->status = PROCESS_ZOMBIE;
  tcb* localWatcher = (tcb*)targetProc->prezombieWatcher;
  GlobalUnlockR(&targetProc->prezombieWatcherLock);
  // Notify zombie watcher
  if (localWatcher) {
    LocalLockR();
    assert(localWatcher->status == THREAD_BLOCKED);
    // Only spin on multicore
    while (!__sync_bool_compare_and_swap(
        &localWatcher->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD))
      ;
    addToXLX(localWatcher);
    localWatcher->status = THREAD_RUNNABLE;
    swtichToThread_Prelocked(localWatcher);
  }
}

// Run inside reaper.
// So we don't want to acquire any kmutex
// Must guarantee there's no thread alive anymore
void reapProcess(pcb* targetProc) {
  KERNEL_STACK_CHECK;
  freeUserspace(targetProc->pd);
  freePageDirectory(targetProc->pd);
  turnToZombie(targetProc);
}

// The first step for the death of a thread. Must be the running thread.
void suicideThread(tcb* targetThread) {
  KERNEL_STACK_CHECK;
  pcb* targetProc = targetThread->process;
  kmutexWLock(&targetProc->mutex);
  int ntleft = --targetProc->numThread;
  if (ntleft == 0) {
    turnToPreZombie(targetProc);
    targetThread->lastThread = true;
  }
  kmutexWUnlock(&targetProc->mutex);
}

// Run inside reaper.
// So we don't want to acquire any kmutex
// targetThread mustn't be current thread, and current CPU need to own it,
// just like the state before context switch. However, it differs that LocalLock
// does not have to be acquried: reaper is free be interrupted
void reapThread(tcb* targetThread) {
  KERNEL_STACK_CHECK;
  assert(targetThread->status == THREAD_DEAD);
  targetThread->status = THREAD_REAPED;

  // Deregister it self from the process
  if (targetThread->lastThread) {
    assert(targetThread->process->status == PROCESS_PREZOMBIE);
    reapProcess(targetThread->process);
  }

  sfree((void*)targetThread->kernelStackPage, PAGE_SIZE);
  removeTCB(targetThread);
}
