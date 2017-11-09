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

static uint32_t freeUserspace_EachPage(int pdIndex, int ptIndex, PTE* ptentry,
    uint32_t _) {
  uint32_t physicalPage = PE_DECODE_ADDR(*ptentry);
  *ptentry &= ~PE_PRESENT(1);
  freeUserMemPage(physicalPage);

  return 0;
}

void freeUserspace(PageDirectory pd) {
  traverseEntryPageDirectory(pd,
                             STRIP_PD_INDEX(USER_MEM_START),
                             STRIP_PD_INDEX(0xffffffff),
                             freeUserspace_EachPage,
                             0);
}

static pcb** followZombieChain(pcb* proc, int* len) {
  pcb** ret;
  if (len) *len = 0;
  for (ret = &proc->zombieChain; *ret != NULL; ret = &(*ret)->zombieChain) {
    if (len) (*len)++;
  }
  return ret;
}

// Must Lock proc
static void notifyWaiter(pcb* proc, int num) {
  while (proc->waiter && num > 0) {
    // wake up one waiter, need not to own it
    tcb* wThread = (tcb*)proc->waiter->thread;
    assert(wThread->status == THREAD_BLOCKED);
    wThread->status = THREAD_RUNNABLE;
    proc->waiter = proc->waiter->next;
    num--;
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
    int zombieCount;
    followZombieChain(targetProc, &zombieCount);
    pcb** chainEnds = followZombieChain(initPCB, NULL);
    *chainEnds = targetProc->zombieChain;

    initPCB->unwaitedChildProc += zombieCount;
    if (zombieCount > 0) {
      lprintf("Handling %d zombie children to INIT.., now it's %d",
          zombieCount, initPCB->unwaitedChildProc);
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
    lprintf("I'm orphan... contacting INIT, now it's %d",
        directParentPCB->unwaitedChildProc);
  }
  notifyWaiter(directParentPCB, 1);
  kmutexWUnlock(&directParentPCB->mutex);
  releaseEphemeralAccessProcess(directParentPCB);

  // We are done!
}

// Must guarantee there's no thread alive anymore
// Then there's no need to acquire procMutex
void reapProcess(pcb* targetProc) {
  freeUserspace(targetProc->pd);
  freePageDirectory(targetProc->pd);
  turnToZombie(targetProc);
}

// targetThread mustn't be current thread, and current CPU need to own it,
// just like the state before context switch. However, it differs that LocalLock
// does not have to be acquried: reaper is free be interrupted
void reapThread(tcb* targetThread) {
  assert(targetThread->status == THREAD_DEAD);
  targetThread->status = THREAD_REAPED;

  // Deregister it self from the process
  pcb* targetProc = targetThread->process;
  kmutexWLock(&targetProc->mutex);
  int ntleft = --targetProc->numThread;
  kmutexWUnlock(&targetProc->mutex);
  if (ntleft == 0) {
    reapProcess(targetProc);
  }

  sfree((void*)targetThread->kernelStackPage, PAGE_SIZE);
  removeTCB(targetThread);
}
