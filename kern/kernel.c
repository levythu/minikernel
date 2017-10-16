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

#include "driver.h"
#include "loader.h"
#include "set_register.h"
#include "mode_switch.h"
#include "vm.h"
#include "pm.h"
#include "process.h"
#include "cpu.h"
#include "zeus.h"
#include "syscall.h"

void _tickback(unsigned int _) {
  return;
}

void RunInit(const char* filename) {
  tcb* firstThread;
  pcb* firstProc = SpawnProcess(&firstThread);
  activatePageDirectory(firstProc->pd);
  if (LoadELFToProcess(firstProc, firstThread, filename) < 0) {
    panic("RunInit: Fail to init the first process");
  }
  getLocalCPU()->runningPID = firstProc->id;
  getLocalCPU()->runningTID = firstThread->id;
  uint32_t neweflags = (get_eflags() | EFL_RESV1) & ~EFL_AC;
  lprintf("Into Ring3...");

  switchToRing3(firstThread->regs.esp, neweflags, firstThread->regs.eip);
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

    if (handler_install(_tickback) != 0) {
      panic("Fail to install all drivers");
    }
    lprintf("Drivers installed");

    enablePaging();
    claimUserMem();

    initProcess();

    initSyscall();

    RunInit("loader_test2");

    while (1) {
        continue;
    }

    return 0;
}
