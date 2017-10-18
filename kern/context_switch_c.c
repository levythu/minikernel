/** @file context_switch_c.c
 *
 *  @brief C Infrastructure for context switch
 *
 *  C part will do everything apart from core switch (change the register set)
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <x86/cr.h>

#include "x86/asm.h"
#include "x86/cr.h"
#include "common_kern.h"
#include "cpu.h"
#include "vm.h"
#include "bool.h"
#include "context_switch.h"

// Switch to the process pointed by parameter, it will do several things:
// It's not a public function
// - Change the local CPU settings
// - Activate the page directory of that process
// NOTE: must be protected under LocalLock.
static void switchToProcess(pcb* process) {
  activatePageDirectory(process->pd);
  getLocalCPU()->runningPID = process->id;
}

// Swtich to the thread pointed by parameter, also switch the process if needed
// NOTE Current CPU must own the current thread and the thread to switch
// It will do several things:
// - Switch the process, if possible
// - Switch the kernel stack
// - Maintain the states for two TCBs
// - Change the local CPU settings
// - Switch the world to the new thread
// It's also the entry when a descheduled thread is switched-on, and it will
// looks like returnning from switchTheWorld() call.
// - Disown the former thread
//
// The function is interrupt-safe (it acquires locak lock)
void swtichToThread(tcb* thread) {
  LocalLockR();
  cpu* core = getLocalCPU();
  if (thread->process->id != core->runningPID) {
    switchToProcess(thread->process);
  }

  set_esp0(thread->kernelStackPage + PAGE_SIZE - 1);

  ureg_t dummyUReg;
  ureg_t* currentUReg = &dummyUReg;
  if (core->runningTID != -1) {
    tcb* cThread = findTCB(core->runningTID);
    if (!cThread) {
      panic("swtichToThread: fail to find the thread with id = %d",
            core->runningTID);
    }
    currentUReg = &cThread->regs;
    cThread->status = THREAD_RUNNABLE;
  }
  core->runningTID = thread->id;
  thread->status = THREAD_RUNNING;
  tcb* switchedFrom =
      (tcb*)switchTheWorld(currentUReg, &thread->regs, (int)thread);

  // disown the former thread
  switchedFrom->ownerCPU = -1;
  // Now it should be where the entry of newly switched thread.
  // Also notice that the init thread (thread #1) is special, it will not get
  // out at this place!!
  LocalUnlockR();
}
