/** @file kernel.c
 *  @brief An initial kernel.c
 *
 *  You should initialize things in kernel_main(),
 *  and then run stuff.
 *
 *  @author Harry Q. Bovik (hqbovik)
 *  @author Fred Hacker (fhacker)
 *  @bug No known bugs.
 */

#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <stdlib.h>
#include <simics.h>                 /* lprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */
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

extern void initMemManagement();

void _tickback(unsigned int tk) {
  if (tk % 1000 == 0) {
    lprintf("tick");
  }
  return;
}

void _keyboardback() {
  if (readchar() != 's') return;
  int currentTID = getLocalCPU()->runningTID;
  if (currentTID == -1) return;

  tcb* nextThread = pickNextRunnableThread(findTCB(currentTID));
  if (nextThread == NULL) return;
  lprintf("Scheduling to thread #%d", nextThread->id);
  swtichToThread(nextThread);
}

void RunInit(const char* filename, pcb* firstProc, tcb* firstThread) {
  // From swtichToThread, so we must unlock.
  LocalUnlockR();

  lprintf("Hello from the 1st thread! The init program is: %s", filename);

  uint32_t esp, eip;
  if (LoadELFToProcess(firstProc, firstThread, filename, &eip, &esp) < 0) {
    panic("RunInit: Fail to init the first process");
  }

  uint32_t neweflags =
      (get_eflags() | EFL_RESV1 | EFL_IF) & ~EFL_AC;
  lprintf("Into Ring3...");

  switchToRing3(esp, neweflags, eip);
}

void EmitInitProcess(const char* filename) {
  tcb* firstThread;
  pcb* firstProc = SpawnProcess(&firstThread);
  firstThread->regs.eip = (uint32_t)RunInit;
  firstThread->regs.esp = firstThread->kernelStackPage + PAGE_SIZE - 1;

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

    if (handler_install(_tickback, _keyboardback) != 0) {
      panic("Fail to install all drivers");
    }
    lprintf("Drivers installed");

    enablePaging();
    claimUserMem();

    initProcess();

    // TODO set more exception handler!
    initSyscall();

    EmitInitProcess("exec_basic");

    while (1) {
        continue;
    }

    return 0;
}
