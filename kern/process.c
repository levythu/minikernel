/** @file process.c
 *
 *  @brief TODO

 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <stdlib.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>

#include "x86/asm.h"
#include "x86/cr.h"
#include "common_kern.h"
#include "process.h"
#include "bool.h"
#include "vm.h"
#include "cpu.h"

static pcb* pcbList;
static tcb* tcbList;
static int pidNext;
static int tidNext;

static CrossCPULock latch;

void initProcess() {
  initCrossCPULock(&latch);
  pcbList = NULL;
  tcbList = NULL;
  pidNext = tidNext = 1;
}

static pcb** _findPCB(int pid) {
  GlobalLockR(&latch);
  pcb** ptr;
  for (ptr = &pcbList; *ptr != NULL; ptr = &(*ptr)->next ) {
    if ((*ptr)->id == pid) break;
  }
  GlobalUnlockR(&latch);
  return ptr;
}

pcb* findPCB(int pid) {
  return *_findPCB(pid);
}

static void _removePCB(pcb* proc) {
  pcb** ptrToProcToDelete = _findPCB(proc->id);
  assert(*ptrToProcToDelete != NULL); // Must find it
  lprintf("Removing process #%d", proc->id);
  *ptrToProcToDelete = proc->next;
  // Goodbye, my process
  sfree(proc, sizeof(pcb));
}

void removePCB(pcb* proc) {
  GlobalLockR(&latch);
  proc->_hasAbandoned = true;
  if (proc->_ephemeralRefCount == 0) {
    _removePCB(proc);
  }
  GlobalUnlockR(&latch);
}

pcb* findPCBWithEphemeralAccess(int pid) {
  GlobalLockR(&latch);
  pcb* ret = findPCB(pid);
  if (ret) ret->_ephemeralRefCount++;
  GlobalUnlockR(&latch);
  return ret;
}

void releaseEphemeralAccessProcess(pcb* proc) {
  GlobalLockR(&latch);
  assert(proc->_ephemeralRefCount > 0);
  proc->_ephemeralRefCount--;
  if (proc->_ephemeralRefCount == 0 && proc->_hasAbandoned) {
    _removePCB(proc);
  }
  GlobalUnlockR(&latch);
}

pcb* newPCB() {
  GlobalLockR(&latch);
  pcb* npcb = (pcb*)smalloc(sizeof(pcb));
  if (!npcb) {
    panic("newPCB: fail to get space for new PCB.");
  }
  npcb->id = pidNext++;
  npcb->next = pcbList;
  pcbList = npcb;

  npcb->_ephemeralRefCount = 0;
  npcb->_hasAbandoned = false;
  GlobalUnlockR(&latch);
  return npcb;
}

static tcb** _findTCB(int tid) {
  GlobalLockR(&latch);
  tcb** ptr;
  for (ptr = &tcbList; *ptr != NULL; ptr = &(*ptr)->next ) {
    if ((*ptr)->id == tid) break;
  }
  GlobalUnlockR(&latch);
  return ptr;
}

tcb* findTCB(int tid) {
  return *_findTCB(tid);
}

tcb* findTCBWithEphemeralAccess(int tid) {
  GlobalLockR(&latch);
  tcb* ret = findTCB(tid);
  if (ret) ret->_ephemeralRefCount++;
  GlobalUnlockR(&latch);
  return ret;
}

tcb* newTCB() {
  GlobalLockR(&latch);
  tcb* ntcb = (tcb*)smalloc(sizeof(tcb));
  if (!ntcb) {
    panic("newTCB: fail to get space for new TCB.");
  }
  ntcb->owned = THREAD_NOT_OWNED;
  ntcb->status = THREAD_UNINITIALIZED;
  ntcb->id = tidNext++;
  ntcb->next = tcbList;
  tcbList = ntcb;

  ntcb->_ephemeralRefCount = 0;
  ntcb->_hasAbandoned = false;
  GlobalUnlockR(&latch);
  return ntcb;
}

tcb* roundRobinNextTCBWithEphemeralAccess(tcb* thread,
    bool needToReleaseFormer) {
  GlobalLockR(&latch);
  tcb* retTCB = roundRobinNextTCB(thread);
  if (retTCB != thread) {
    // ephemeral refer to this thread
    retTCB->_ephemeralRefCount++;
    if (needToReleaseFormer) {
      releaseEphemeralAccess(thread);
    }
  }
  GlobalUnlockR(&latch);
  return retTCB;
}

// Must work with latch aquired
// Process module is not responsible for freeing kernel stack, relevant process
// etc. Refer to Zeus for those jobs
static void _removeTCB(tcb* thread) {
  tcb** ptrToThreadToDelete = _findTCB(thread->id);
  assert(*ptrToThreadToDelete != NULL); // Must find it
  *ptrToThreadToDelete = thread->next;
  lprintf("Removing thread #%d", thread->id);
  // Goodbye, my thread
  sfree(thread, sizeof(tcb));
}

void releaseEphemeralAccess(tcb* thread) {
  GlobalLockR(&latch);
  assert(thread->_ephemeralRefCount > 0);
  thread->_ephemeralRefCount--;
  if (thread->_ephemeralRefCount == 0 && thread->_hasAbandoned) {
    _removeTCB(thread);
  }
  GlobalUnlockR(&latch);
}

void removeTCB(tcb* thread) {
  GlobalLockR(&latch);
  thread->_hasAbandoned = true;
  if (thread->_ephemeralRefCount == 0) {
    _removeTCB(thread);
  }
  GlobalUnlockR(&latch);
}

tcb* roundRobinNextTCB(tcb* thread) {
  GlobalLockR(&latch);
  if (!thread->next) {
    thread = tcbList;
  } else {
    thread = thread->next;
  }
  GlobalUnlockR(&latch);
  return thread;
}

tcb* roundRobinNextTCBID(int tid) {
  GlobalLockR(&latch);
  tcb* cTCB = findTCB(tid);
  if (!cTCB) {
    panic("roundRobinNextTCB: trying to round robin a non-existing thread");
  }
  if (!cTCB->next) {
    cTCB = tcbList;
  } else {
    cTCB = cTCB->next;
  }
  GlobalUnlockR(&latch);
  return cTCB;
}
