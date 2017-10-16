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

static pcb* pcbList;
static tcb* tcbList;
static int pidNext;
static int tidNext;


void initProcess() {
  pcbList = NULL;
  tcbList = NULL;
  pidNext = tidNext = 1;
}

static pcb** _findPCB(int pid) {
  pcb** ptr;
  for (ptr = &pcbList; *ptr != NULL; ptr = &(*ptr)->next ) {
    if ((*ptr)->id == pid) break;
  }
  return ptr;
}

pcb* findPCB(int pid) {
  return *_findPCB(pid);
}

pcb* newPCB() {
  pcb* npcb = (pcb*)smalloc(sizeof(pcb));
  if (!npcb) {
    panic("newPCB: fail to get space for new PCB.");
  }
  npcb->id = pidNext++;
  npcb->next = pcbList;
  pcbList = npcb;
  return npcb;
}

static tcb** _findTCB(int tid) {
  tcb** ptr;
  for (ptr = &tcbList; *ptr != NULL; ptr = &(*ptr)->next ) {
    if ((*ptr)->id == tid) break;
  }
  return ptr;
}

tcb* findTCB(int tid) {
  return *_findTCB(tid);
}

tcb* newTCB() {
  tcb* ntcb = (tcb*)smalloc(sizeof(tcb));
  if (!ntcb) {
    panic("newTCB: fail to get space for new TCB.");
  }
  ntcb->id = tidNext++;
  ntcb->next = tcbList;
  tcbList = ntcb;
  return ntcb;
}
