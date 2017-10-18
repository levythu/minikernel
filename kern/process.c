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

pcb* newPCB() {
  GlobalLockR(&latch);
  pcb* npcb = (pcb*)smalloc(sizeof(pcb));
  if (!npcb) {
    panic("newPCB: fail to get space for new PCB.");
  }
  npcb->id = pidNext++;
  npcb->next = pcbList;
  pcbList = npcb;
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

tcb* newTCB() {
  GlobalLockR(&latch);
  tcb* ntcb = (tcb*)smalloc(sizeof(tcb));
  if (!ntcb) {
    panic("newTCB: fail to get space for new TCB.");
  }
  ntcb->ownerCPU = -1;
  ntcb->status = THREAD_UNINITIALIZED;
  ntcb->id = tidNext++;
  ntcb->next = tcbList;
  tcbList = ntcb;
  GlobalUnlockR(&latch);
  return ntcb;
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
