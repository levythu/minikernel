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

// Will own the thread
pcb* SpawnProcess(tcb** firstThread) {
  pcb* npcb = newPCB();
  npcb->pd = newPageDirectory();
  setKernelMapping(npcb->pd);

  tcb* ntcb = newTCB();
  int cpuid = getLocalCPU()->id;
  // Only happens in multi-cpu
  while (!__sync_bool_compare_and_swap(&ntcb->ownerCPU, -1, cpuid))
    ;
  ntcb->process = npcb;
  ntcb->kernelStackPage = (uint32_t)smalloc(PAGE_SIZE);
  if (!ntcb->kernelStackPage) {
    panic("SpawnProcess: fail to create kernel stack for new process.");
  }
  ntcb->status = THREAD_INITIALIZED;

  *firstThread = ntcb;
  return npcb;
}

int LoadELFToProcess(pcb* proc, tcb* firstThread, const char* fileName) {
  if (initELFMemory(fileName,
        proc->pd, &proc->memMeta,
        (uint32_t*)&firstThread->regs.eip,
        (uint32_t*)&firstThread->regs.esp) < 0) {
    return -1;
  }
  return 0;
}
