/** @file kernel.c
 *  @brief The entry from bootstrap, doing all kernel initializing things, and
 *  make the 1st process, load init()
 *
 *  @author Leiyu Zhao
 */

#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <stdlib.h>
#include <simics.h>
#include <assert.h>

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>
#include <x86/eflags.h>
#include <x86/cr.h>

#include "driver.h"
#include "loader.h"
#include "mode_switch.h"
#include "vm.h"
#include "pm.h"
#include "process.h"
#include "cpu.h"
#include "zeus.h"
#include "syscall.h"
#include "context_switch.h"
#include "scheduler.h"
#include "keyboard_driver.h"
#include "keyboard_event.h"
#include "dbgconf.h"
#include "sysconf.h"
#include "fault.h"
#include "timeout.h"
#include "kernel_stack_protection.h"
#include "virtual_console.h"

#include "hv.h"

extern void initMemManagement();

// Dummy timer event, can visualize whether the interrupt is on.
void _tickback(unsigned int tk) {
  KERNEL_STACK_CHECK;
  onTickEvent();
  #ifdef CONTEXT_SWTICH_ON_RIGHT_KEY
    if (tk % 1000 == 0) {
      lprintf("tick");
    }
  #else
    tcb* currentThread = findTCB(getLocalCPU()->runningTID);
    if (currentThread && !currentThread->descheduling) {
      yieldToNext();
      hv_CallMeOnTick(&currentThread->process->hyperInfo);
    }
  #endif
}

// The init function that runs inside first kernel stack.
// It is a special entry after swtichTheWorld, so it will do conventional
// clean-ups (turn on interrupt, disown last thread)
// Also it will do a fork, one for running specified program and another one
// running idle.
// It runs program by standard execProcess() (the same with exec() syscall)
void RunInit(const char* filename, pcb* firstProc, tcb* firstThread) {
  // From swtichToThread, so we must unlock.
  // But since it switches from -1, so no need to disown anything
  LocalUnlockR();

  lprintf("Hello from the 1st thread! The init program is: %s", filename);

  // Fork an idle
  if (forkProcess(firstThread) == 0) {
    // new thread, Will go into ring3

    tcb* currentThread = findTCB(getLocalCPU()->runningTID);

    // This is INIT thread, assert it
    assert(currentThread->process->id == INIT_PID);
    execProcess(currentThread, filename, NULL);
    panic("RunInit: fail to run the 1st process");
  } else {
    // parent thread, Will go into ring3
    tcb* currentThread = findTCB(getLocalCPU()->runningTID);
    execProcess(currentThread, "idle", NULL);
    panic("RunInit: fail to run the 1st process");
  }
}

// Emit the first process, construct its initial kernel stack so that scheduler
// can control it. Filename is the first process to launch. However, another
// idle will be forked too, to keep scheduler always have sth. to schedule.
void EmitInitProcess(const char* filename, int firstVC) {
  tcb* firstThread;
  pcb* firstProc = SpawnProcess(&firstThread);
  firstProc->vcNumber = firstVC;
  referVirtualConsole(firstProc->vcNumber);
  firstThread->regs.eip = (uint32_t)RunInit;
  firstThread->regs.esp = firstThread->kernelStackPage + PAGE_SIZE - 1;
  firstThread->regs.ebp = 0;    // we set ebp initialized to zero

  uint32_t* futureStack = (uint32_t*)firstThread->regs.esp;
  futureStack[-1] = (uint32_t)firstThread;
  futureStack[-2] = (uint32_t)firstProc;
  futureStack[-3] = (uint32_t)filename;
  futureStack[-4] = 0xdeadbeef;   // invalid ret address of root call frame
  firstThread->regs.esp = (uint32_t)&futureStack[-4];

  // This is a one way trip!
  swtichToThread(firstThread);
}

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    lprintf("Hello from a brand new kernel!");

    initCPU();
    initMemManagement();

    int firstVC = initVirtualConsole();

    initTimeout();
    if (handler_install(_tickback, onKeyboardSync, onKeyboardAsync) != 0) {
      panic("Fail to install all drivers");
    }
    lprintf("Drivers installed");

    claimUserMem();
    enablePaging();

    initProcess();

    registerFaultHandler();
    initSyscall();

    initHypervisor();

    EmitInitProcess("init", firstVC);

    while (1) {
        continue;
    }

    return 0;
}
