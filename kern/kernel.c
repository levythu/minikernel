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

#include "driver.h"
#include "loader.h"
#include "vm.h"
#include "pm.h"

void _tickback(unsigned int _) {
  return;
}

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    lprintf("Hello from a brand new kernel!");
    if (handler_install(_tickback) != 0) {
      panic("Fail to install all drivers");
    }
    lprintf("Drivers installed");

    enablePaging();
    claimUserMem();

    loadFile("init");

    while (1) {
        continue;
    }

    return 0;
}
