/** @file zeus.c
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
#include "process.h"
#include "vm.h"
#include "loader.h"


pcb* SpawnProcess(tcb** firstThread) {
  pcb* npcb = newPCB();
  npcb->pd = newPageDirectory();
  setKernelMapping(npcb->pd);

  tcb* ntcb = newTCB();
  ntcb->process = npcb;

  *firstThread = ntcb;
  return npcb;
}

int LoadELFToProcess(pcb* proc, tcb* firstThread, const char* fileName) {
  if (initELFMemory("loader_test2",
        proc->pd, &proc->memMeta,
        (uint32_t*)&firstThread->regs.eip,
        (uint32_t*)&firstThread->regs.esp) < 0) {
    return -1;
  }
  return 0;
}
