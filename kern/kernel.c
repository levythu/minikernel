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
#include <simics.h>                 /* lprintf() */

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

extern void initMemManagement();

void _tickback(unsigned int tk) {
  if (tk % 1000 == 0) {
    lprintf("tick");
  }
  return;
}

void RunInit(const char* filename, pcb* firstProc, tcb* firstThread) {
  // From swtichToThread, so we must unlock.
  // But since it switches from -1, so no need to disown anything
  LocalUnlockR();

  lprintf("Hello from the 1st thread! The init program is: %s", filename);

  // Fork an idle
  if (forkProcess(firstThread) == 0) {
    // new thread, Will go into ring3
    char tmp[123];
    getStringBlocking(tmp, 123);
    
    tcb* currentThread = findTCB(getLocalCPU()->runningTID);
    execProcess(currentThread, filename, NULL);
    panic("RunInit: fail to run the 1st process");
  } else {
    // parent thread, Will go into ring3
    tcb* currentThread = findTCB(getLocalCPU()->runningTID);
    execProcess(currentThread, "idle", NULL);
    panic("RunInit: fail to run the 1st process");
  }
}

void EmitInitProcess(const char* filename) {
  tcb* firstThread;
  pcb* firstProc = SpawnProcess(&firstThread);
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

    initKeyboardEvent();
    if (handler_install(_tickback, onKeyboardSync, onKeyboardAsync) != 0) {
      panic("Fail to install all drivers");
    }
    lprintf("Drivers installed");

    enablePaging();
    claimUserMem();

    initProcess();

    // TODO set more exception handler!
    initSyscall();

    EmitInitProcess("fork_test1");

    while (1) {
        continue;
    }

    return 0;
}
