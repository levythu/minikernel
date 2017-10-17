/** @file context_switch_c.c
 *
 *  @brief TODO

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

static void switchToProcess(pcb* process) {
  activatePageDirectory(process->pd);
  getLocalCPU()->runningPID = process->id;
}

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
  }
  switchTheWorld(currentUReg, &thread->regs);
  // Now it should be where the entry of newly switched thread.
  // Also notice that the init thread (thread #1) is special, it will not get
  // out at this place!!
  LocalUnlockR();
}
