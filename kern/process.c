/** @file process.c
 *
 *  @brief Core data structure for PCB and TCB.
 *
 *  See process.h for very detailed docs
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
#include "dbgconf.h"

static pcb* pcbList;
static tcb* tcbList;
static int pidNext;
static int tidNext;

static CrossCPULock latch;

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
  GlobalLockR(&latch);
  pcb* ret = *_findPCB(pid);
  GlobalUnlockR(&latch);
  return ret;
}

static void _removePCB(pcb* proc) {
  pcb** ptrToProcToDelete = _findPCB(proc->id);
  assert(*ptrToProcToDelete != NULL); // Must find it
  #ifdef VERBOSE_PRINT
  lprintf("Removing process #%d", proc->id);
  #endif
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

/*****************************************************************************/

static dualLinklist xlx;  // express links

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
  GlobalLockR(&latch);
  tcb* ret = *_findTCB(tid);
  GlobalUnlockR(&latch);
  return ret;
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
  ntcb->_xlx.next = ntcb->_xlx.prev = NULL;
  ntcb->_xlx.data = ntcb;
  GlobalUnlockR(&latch);
  return ntcb;
}

void removeFromXLX(tcb* thread) {
  GlobalLockR(&latch);
  assert(thread->_xlx.next != NULL);
  assert(thread->_xlx.prev != NULL);
  thread->_xlx.next->prev = thread->_xlx.prev;
  thread->_xlx.prev->next = thread->_xlx.next;
  thread->_xlx.next = thread->_xlx.prev = NULL;
  GlobalUnlockR(&latch);
}

void addToXLX(tcb* thread) {
  GlobalLockR(&latch);
  assert(thread->_xlx.next == NULL);
  assert(thread->_xlx.prev == NULL);
  thread->_xlx.next = xlx.next;
  thread->_xlx.prev = &xlx;
  thread->_xlx.next->prev = &thread->_xlx;
  thread->_xlx.prev->next = &thread->_xlx;
  GlobalUnlockR(&latch);
}

tcb* dequeueXLX() {
  GlobalLockR(&latch);
  dualLinklist* last = xlx.prev;
  if (last == &xlx) {
    GlobalUnlockR(&latch);
    return NULL;
  }
  assert(last->data != NULL);
  tcb* t = (tcb*)last->data;
  removeFromXLX(t);
  GlobalUnlockR(&latch);
  return t;
}

tcb* roundRobinNextTCBWithEphemeralAccess(tcb* thread,
    bool needToReleaseFormer) {
  GlobalLockR(&latch);
  tcb* retTCB = thread;
  while (retTCB == thread) {
    if (retTCB->_xlx.prev == &xlx && retTCB->_xlx.next == &xlx) {
      // retTCB is the only runnable. return it
      break;
    }
    // refind one
    retTCB = roundRobinNextTCB(retTCB);
    if (retTCB == NULL) {
      GlobalUnlockR(&latch);
      return thread;
    }
  }
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

  if (thread->_xlx.next) {
    removeFromXLX(thread);
  }
  #ifdef VERBOSE_PRINT
  lprintf("Removing thread #%d", thread->id);
  #endif
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
  thread = dequeueXLX();
  assert(thread != NULL);  // must have at least one runnable (i.e. idle)
  addToXLX(thread);
  GlobalUnlockR(&latch);
  return thread;
}

void initProcess() {
  initCrossCPULock(&latch);
  pcbList = NULL;
  tcbList = NULL;
  pidNext = tidNext = 1;

  xlx.prev = xlx.next = &xlx;
}

/*****************************************************************************/
// For debugging

static const char* ProcessStatusToString(ProcessStatus s) {
  if (s == PROCESS_INITIALIZED) return "RUN";
  if (s == PROCESS_PREZOMBIE) return "PRZ";
  if (s == PROCESS_ZOMBIE) return "ZOM";
  if (s == PROCESS_DEAD) return "NUL";
  return "???";
}
static const char* ThreadStatusToString(ThreadStatus s) {
  if (s == THREAD_UNINITIALIZED) return "NEW";
  if (s == THREAD_INITIALIZED) return "IOK";
  if (s == THREAD_RUNNABLE) return "W8T";
  if (s == THREAD_BLOCKED) return "BLK";
  if (s == THREAD_RUNNING) return "RUN";
  if (s == THREAD_DEAD) return "DIE";
  if (s == THREAD_REAPED) return "NUL";
  if (s == THREAD_BLOCKED_USER) return "DSC";
  return "???";
}
void reportProcessAndThread() {
  GlobalLockR(&latch);
    lprintf("├ Process List");
  int totCount = 0;
  for (pcb* proc = pcbList; proc != NULL; proc = proc->next ) {
    totCount++;
    lprintf("│ ├ [%4d <- %4d] (%s) fTID:%d, nThr:%d, wCh:%d %s",
                        proc->id,
                        proc->parentPID,
                        ProcessStatusToString(proc->status),
                        proc->firstTID,
                        proc->numThread,
                        proc->unwaitedChildProc,
                        proc->hyperInfo.isHyper ? "(VirtualMachine)" : "");
  }
    lprintf("│ └ Total %d processes", totCount);

    lprintf("├ Thread List");
  totCount = 0;
  for (tcb* thr = tcbList; thr != NULL; thr = thr->next ) {
    totCount++;
    lprintf("│ ├ [%4d <- %4d] (%s) isOwned:%s",
                        thr->id,
                        thr->process->id,
                        ThreadStatusToString(thr->status),
                        thr->owned == THREAD_NOT_OWNED ? "F" : "T");
  }
    lprintf("│ └ Total %d threads", totCount);

    lprintf("├ Express Links");
  totCount = 0;
  for (dualLinklist* lk = xlx.next; lk != &xlx; lk = lk->next ) {
    totCount++;
    lprintf("│ ├ Threads #%d", ((tcb*)lk->data)->id);
  }
    lprintf("│ └ Total %d threads", totCount);
  GlobalUnlockR(&latch);
}
