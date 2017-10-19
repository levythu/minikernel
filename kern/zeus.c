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
#include <string.h>

#include "x86/asm.h"
#include "x86/cr.h"
#include "common_kern.h"
#include "process.h"
#include "bool.h"
#include "process.h"
#include "vm.h"
#include "pm.h"
#include "loader.h"
#include "context_switch.h"

// Will own the thread
pcb* SpawnProcess(tcb** firstThread) {
  pcb* npcb = newPCB();
  npcb->pd = newPageDirectory();
  setKernelMapping(npcb->pd);
  npcb->parentPID = -1;

  tcb* ntcb = newTCB();
  // Only happens in multi-cpu
  while (!__sync_bool_compare_and_swap(
      &ntcb->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD))
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

static uint32_t rebuildPD_EachPage(int pdIndex, int ptIndex, PTE* ptentry,
    uint32_t buffer) {
  if (!buffer) {
    // There's some failure. We cannot fork anymore, instead, we remove
    // the reference to physical page (because that belongs to my parent)
    *ptentry = PTE_CLEAR_ADDR(*ptentry);
    *ptentry &= ~PE_PRESENT(1);
    return 0;
  } else {
    uint32_t pageAddr = RECONSTRUCT_ADDR(pdIndex, ptIndex);
    // Move things into buffer, change mapping, flush TLB, put things back
    memcpy((void*)buffer, (void*)pageAddr, PAGE_SIZE);

    uint32_t newPage = getUserMemPage();
    if (newPage == 0) {
      // no enough user space, abort
      *ptentry = PTE_CLEAR_ADDR(*ptentry);
      *ptentry &= ~PE_PRESENT(1);
      return 0;
    }

    *ptentry = PTE_CLEAR_ADDR(*ptentry) | newPage;
    invalidateTLB(pageAddr);

    memcpy((void*)pageAddr, (void*)buffer, PAGE_SIZE);
    // Done! Passing in the buffer address to go further
    return buffer;
  }
}

// mypd must be active pd
// Rebuild (duplicate the page directoy), return whether success
// If any failure, all page from parent pd will be discarded and newly allocated
// will be kept, so that it's safe to free
static bool rebuildPD(PageDirectory mypd) {
        lprintf("build new PD!");
  uint32_t buffer = (uint32_t)smalloc(PAGE_SIZE);
  if (buffer == 0) {
    panic("rebuildPD: no kernel space to rebuild page table.");
  }
  uint32_t success =
      traverseEntryPageDirectory(mypd,
                                 STRIP_PD_INDEX(USER_MEM_START),
                                 STRIP_PD_INDEX(0xffffffff),
                                 rebuildPD_EachPage,
                                 buffer);
  sfree((void*)buffer, PAGE_SIZE);
  if (!success) {
    lprintf("Fail to rebuild new page directory, no enough user space");
    return false;
  }
  return true;
}

// Fork a new process, based on the process of current thread.
// NOTE: the current process *must* only have the current thread
// and of course, currentThread must be owned by the CPU
void forkProcess(tcb* currentThread) {
  tcb* newThread;
  pcb* newProc = SpawnProcess(&newThread);
  pcb* currentProc = currentThread->process;

  // proc-related
  clonePageDirectory(currentProc->pd, newProc->pd,
                     STRIP_PD_INDEX(USER_MEM_START),
                     STRIP_PD_INDEX(0xffffffff));
  newProc->memMeta = currentProc->memMeta;
  newProc->parentPID = currentProc->id;

  // thread-related
  newThread->regs = currentThread->regs;
  // block the current thread, forbid it run until the new finish copying the
  // pages.
  currentThread->status = THREAD_BLOCKED;
  if (!checkpointTheWorld(&newThread->regs,
      currentThread->kernelStackPage, newThread->kernelStackPage, PAGE_SIZE)) {
    // This is the old process!
    // We want to rebase the newThread, overflow does not matter :)
    newThread->regs.ebp +=
        newThread->kernelStackPage - currentThread->kernelStackPage;
    newThread->regs.esp +=
        newThread->kernelStackPage - currentThread->kernelStackPage;

    // Yield to the new thread now!
    swtichToThread(newThread);
    // We have recover, we know that the new process is forked and finishes copy
    // But we cannot access thread (it may have already ends!)
    // However, newProc is good to access! It's my child, and it cannot die
    // (while maybe zombie) before I wait()
  } else {
    // This is the new process!
    // newProc/newThread is me now.
    // currentProc/currentThread is not me anymore, but we need to disown it.
    currentThread->owned = THREAD_NOT_OWNED;

    // Now it's time to rebuild my page directory
    if (!rebuildPD(newProc->pd)) {
      // TODO kill the process
    }

    // On single core machine it's nothing
    // Even on multicore machine, since currentThread is BLOCKED, no one can
    // really own it, so this is very transient
    while (!__sync_bool_compare_and_swap(
        &currentThread->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD))
      ;

    lprintf("Setting %d to runnable", currentThread->id);
    currentThread->status = THREAD_RUNNABLE;
    currentThread->owned = THREAD_NOT_OWNED;
  }
}


int LoadELFToProcess(pcb* proc, tcb* firstThread, const char* fileName,
    uint32_t* eip, uint32_t* esp) {
  if (initELFMemory(fileName, proc->pd, &proc->memMeta, eip, esp) < 0) {
    return -1;
  }
  return 0;
}
