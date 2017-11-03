/** @file zeus.c
 *
 *  @brief Zeus control every life (process and thread)
 *
 *  This module is used for higher level process and thread lifecycle control.
 *  It contains the spawn, fork, exec, death (a great part for death is
 *  deligated to reaper, see reaper.c) for process and thread. Also it
 *  coordinates different modules to achieve those functionalities
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
#include "scheduler.h"

// High level process creation, and will also spawn one thread for that process
// Initiating every field for the pcb and tcb. Also, it will own the thread, for
// further initial stack setting and context switching.
// NOTE: the thread returned here is not ready for running, so the caller should
// set up everything (precisely, the running stack / registers) before disown it
pcb* SpawnProcess(tcb** firstThread) {
  pcb* npcb = newPCB();
  npcb->pd = newPageDirectory();
  setKernelMapping(npcb->pd);
  npcb->parentPID = -1;
  npcb->numThread = 1;
  npcb->retStatus = -2;
  npcb->zombieChain = NULL;
  npcb->waiter = NULL;
  npcb->status = PROCESS_INITIALIZED;
  kmutexInit(&npcb->mutex);
  kmutexInit(&npcb->memlock);

  LocalLockR();
  tcb* ntcb = newTCB();
  // Spinloop to get lock will only happens in multi-cpu
  while (!__sync_bool_compare_and_swap(
      &ntcb->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD))
    ;
  LocalUnlockR();
  ntcb->process = npcb;
  ntcb->memLockStatus = KMUTEX_NOT_ACQUIRED;
  ntcb->kernelStackPage = (uint32_t)smalloc(PAGE_SIZE);
  if (!ntcb->kernelStackPage) {
    panic("SpawnProcess: fail to create kernel stack for new process.");
  }
  ntcb->status = THREAD_INITIALIZED;

  *firstThread = ntcb;
  npcb->firstTID = ntcb->id;
  return npcb;
}

// The callback function for forking a memory layout.
// It will try to create a new physical page, copy the old one to the new page
// and adjust page table to refer to new page.
// For any failure (no memory available) the subsequent rebuild will turn to
// "destory": i.e., simply tear the old page off the current page table, so that
// when the page table is revoked, all physical pages are dedicated
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

    uint32_t newPage;
    if (isZFOD(PE_DECODE_ADDR(*ptentry))) {
      newPage = getUserMemPageZFOD();
    } else {
      newPage = getUserMemPage();
    }
    if (newPage == 0) {
      // no enough user space, abort
      *ptentry = PTE_CLEAR_ADDR(*ptentry);
      *ptentry &= ~PE_PRESENT(1);
      return 0;
    }

    *ptentry = PTE_CLEAR_ADDR(*ptentry) | newPage;

    // only writeback when it's a non ZFOD data
    if (!isZFOD(PE_DECODE_ADDR(*ptentry))) {
      // Temporary allow write
      PTE savedPTE = *ptentry;
      *ptentry |= PE_WRITABLE(1);
      invalidateTLB(pageAddr);

      memcpy((void*)pageAddr, (void*)buffer, PAGE_SIZE);
      *ptentry = savedPTE;
      invalidateTLB(pageAddr);
    }

    // Done! Passing in the buffer address to go further
    return buffer;
  }
}

// mypd must be active pd
// Rebuild (duplicate the page directoy), return whether success
// If any failure, all page from parent pd will be discarded and newly allocated
// will be kept, so that it's safe to free
static bool rebuildPD(PageDirectory mypd) {
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

// Apply delta to all saved %ebp on the stack, which happens in adjustion phase
// of fork()
// Since the initial ebp is all set to zero, it stops at zero
// [kernelStackStart, kernelStackEnd] is just for verification
void rebuildKernelStack(uint32_t ebpStartChain, uint32_t delta,
    uint32_t kernelStackStart, uint32_t kernelStackEnd) {
  if (ebpStartChain == 0) return;
  uint32_t ebp = ebpStartChain;
  while (true) {
    if (ebp < kernelStackStart || ebp > kernelStackEnd) {
      panic("%%ebp=0x%08lx does not fall into kernel stack [0x%08lx, 0x%08lx]",
            kernelStackStart, kernelStackEnd);
    }
    uint32_t* pNextEBP = (uint32_t*)ebp;
    if (*pNextEBP == 0) return;
    *pNextEBP += delta;
    ebp = *pNextEBP;
  }
}

// Fork a new process, based on the process of current thread. Return zero for
// new process and child tid for old process.
// Fork is completed in two phase:
// 1. Fork phase: fork everything:
//    1.1. Copy all primary members of tcb and pcb
//    1.2. Shallow copy memory directory
//    1.3. Copy register set and kernel stack (use atomic snapshot)
//    1.4. Adjustion: adjust %esp %ebp and the stack-saved %ebps for the new
//         kernel stack.
// Then it freezes the parent process (to avoid memory change), and switch to
// child process to finish phase 2
// 2. Rebuild phase: deep copy page directory, this phase is when two process
//    will have two set of memories. (If COW is implemented, this phase is
//    scattered in subsequent memory access)
// After phase two, the parent process is re-enable.
// NOTE: the current process *must* only have the current thread
// and of course, currentThread must be owned by the CPU
// NOTE: never use it on a kernel-to-kernel interrupt stack. This will lead to
// partial state copied.
int forkProcess(tcb* currentThread) {
  tcb* newThread;
  pcb* newProc = SpawnProcess(&newThread);
  pcb* currentProc = currentThread->process;

  // proc-related
  clonePageDirectory(currentProc->pd, newProc->pd,
                     STRIP_PD_INDEX(USER_MEM_START),
                     STRIP_PD_INDEX(0xffffffff));
  newProc->memMeta = currentProc->memMeta;
  newProc->parentPID = currentProc->id;
  newProc->retStatus = currentProc->retStatus;

  // thread-related
  newThread->regs = currentThread->regs;
  // block the current thread, forbid it run until the new finish copying the
  // pages.

  // Use this to let child access parent's stack
  int retValueForParentCall;
  int* ptr_retValueForParentCall = &retValueForParentCall;

  if (!checkpointTheWorld(&newThread->regs,
      currentThread->kernelStackPage, newThread->kernelStackPage, PAGE_SIZE)) {
    // This is the old process!
    // We want to rebase the newThread, overflow does not matter :)
    newThread->regs.ebp +=
        newThread->kernelStackPage - currentThread->kernelStackPage;
    newThread->regs.esp +=
        newThread->kernelStackPage - currentThread->kernelStackPage;
    rebuildKernelStack(newThread->regs.ebp,
        newThread->kernelStackPage - currentThread->kernelStackPage,
        newThread->kernelStackPage,
        newThread->kernelStackPage + PAGE_SIZE - 1);

    // Yield to the new thread now! Should be atomic deschedule
    LocalLockR();
    currentThread->status = THREAD_BLOCKED;
    swtichToThread_Prelocked(newThread);
    // We have recover, we know that the new process is forked and finishes copy
    // But we cannot access thread (it may have already ends!)
    // However, newProc is good to access! It's my child, and it cannot die
    // (while maybe zombie) before I wait()

    // Now I'm rescheduled, meaning my child has finish forking!
    // So, retValueForParentCall is safely holding the return value (either
    // failure (-1) or its tid)
    return retValueForParentCall;
  } else {
    // This is the new process!
    // newProc/newThread is me now.
    // currentProc/currentThread is not me anymore, but we need to disown it.
    currentThread->owned = THREAD_NOT_OWNED;
    LocalUnlockR();

    // Now it's time to rebuild my page directory
    if (!rebuildPD(newProc->pd)) {
      // TODO kill the process
      // TODO unblock parent
      *ptr_retValueForParentCall = -1;  // Fork fail
      return 0;
    }

    // On single core machine it's nothing
    // Even on multicore machine, since currentThread is BLOCKED, no one can
    // really own it, so this is very transient
    while (!__sync_bool_compare_and_swap(
        &currentThread->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD))
      ;

    // Before unblock parent process, tell him about our success.
    *ptr_retValueForParentCall = newThread->id;

    lprintf("Setting %d to runnable", currentThread->id);
    currentThread->status = THREAD_RUNNABLE;
    currentThread->owned = THREAD_NOT_OWNED;
    return 0;
  }
}

// A simple wrapper of ELF loader
int LoadELFToProcess(pcb* proc, tcb* firstThread, const char* fileName,
    ArgPackage* argpkg, uint32_t* eip, uint32_t* esp) {
  if (initELFMemory(fileName, proc->pd, argpkg, &proc->memMeta, eip, esp) < 0) {
    return -1;
  }
  return 0;
}

// Internal implementation of exec(), load a new elf to the current process
// space. It's idempotent, i.e. can be safely called multiple times on a single
// process, while the excessive memory will be kept (TODO: maybe a bug? need to
// fix it)
// NOTE: the current process *must* only have the current thread
// argpkg can be null, or a smalloc'd array. If it success (no return), argpkg
// will be disposed correctly; otherwise, the caller should dispose it
int execProcess(tcb* currentThread, const char* filename, ArgPackage* argpkg) {
  uint32_t esp, eip;
  if (LoadELFToProcess(
          currentThread->process, currentThread, filename,
          argpkg, &eip, &esp) < 0) {
    return -1;
  }

  if (argpkg) sfree(argpkg, sizeof(ArgPackage));

  uint32_t neweflags =
      (get_eflags() | EFL_RESV1 | EFL_IF) & ~EFL_AC;
  lprintf("Into Ring3...");

  // Will never return!
  switchToRing3(esp, neweflags, eip);
  return 0;
}

// This does nothing but just mark myself as dead.
// All actual work is done by reaper
void terminateThread(tcb* currentThread) {
  currentThread->status = THREAD_DEAD;
  yieldToNext();
}

int waitThread(tcb* currentThread, int* returnCodeAddr) {
  pcb* currentProc = currentThread->process;
  while (true) {
    kmutexWLock(&currentProc->mutex);
    if (currentProc->zombieChain) {
      pcb* zombie = currentProc->zombieChain;
      currentProc->zombieChain = currentProc->zombieChain->zombieChain;
      kmutexWUnlock(&currentProc->mutex);
      // no one will access zombie, so I own the process
      int zombieTID = zombie->firstTID;
      *returnCodeAddr = zombie->retStatus;

      removePCB(zombie);

      return zombieTID;
    } else {
      waiterLinklist wl;
      wl.thread = currentThread;
      wl.next = currentProc->waiter;
      currentProc->waiter = &wl;
      currentThread->status = THREAD_BLOCKED;
      kmutexWUnlock(&currentProc->mutex);
      // Deschedule
      yieldToNext();
    }
  }
  return -1;
}
