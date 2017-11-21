/** @file magic.c
 *
 *  @brief Minimal guest kernel -- uses just lprintf()
 *
 *  @author de0u
 *
 *  Hello is a PebPeb-only executable -- it doesn't work on hardware.
 *
 */

#include <common_kern.h>

/* libc includes. */
#include <malloc.h>
#include <simics.h> /* lprintf() */
#include <stdio.h>

/* multiboot header file */
#include <multiboot.h> /* boot_info */

/* memory includes. */
#include <lmm.h> /* lmm_remove_free() */

/* x86 specific includes */
#include <x86/asm.h> /* enable_interrupts() */
#include <x86/cr.h>
#include <x86/interrupt_defines.h> /* interrupt_setup() */
#include <x86/page.h>
#include <x86/seg.h> /* install_user_segs() */

#include <hvcall.h>
#include <malloc_internal.h>

#include <assert.h>
#include <string.h>

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int
kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp)
{
    lprintf("Well, here I am!");

    if (!hv_isguest()) {
        lprintf("Not a guest?");
        panic("This payload is guest-only");
        return -1;
    }

    if (hv_magic() == HV_MAGIC) {
        lprintf("Ho, ho, ho,");
        lprintf("It's magic, you know...");
        lprintf("Never believe it's not so.");
    } else {
        lprintf("What's the difference between a duck?");
        /* One of its legs are both the same. */
    }

    while (1) {
        continue;
    }
}
