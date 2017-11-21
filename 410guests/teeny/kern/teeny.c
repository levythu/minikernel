/** @file teeny.c
 *
 *  @brief Creates a tiny address space (just big enough
 *         to map the guest-kernel ELF image).
 *  @author de0u
 *
 *  PebPeb-only executable -- it doesn't work on hardware.
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
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    // start, end of kernel image (odd declarations)
    extern unsigned char __kimg_start[];
    extern unsigned char _end[];
    unsigned char *      addr;
    uint32_t *           pt0, *pd;
    int                  p;

    lprintf("Well, here I am!");

    if (!hv_isguest()) {
        lprintf("I am NOT supposed to be here!!!");
        panic("This payload is guest-only");
        return (-1);
    }

    assert(hv_magic() == HV_MAGIC);

    pt0 = _smemalign(PAGE_SIZE, PAGE_SIZE);
    pd  = _smemalign(PAGE_SIZE, PAGE_SIZE);
    if (!pt0 || !pd) {
        panic("Cannot allocate memory");
    }
    bzero(pt0, PAGE_SIZE);
    bzero(pd, PAGE_SIZE);

    addr = (unsigned char *)(((unsigned int)__kimg_start)
                             & ((unsigned int)~0x0FFF));
    p    = (unsigned int)addr >> PAGE_SHIFT;

    lprintf("Addr %p, p %d (end %p)", addr, p, _end);

    while (addr <= _end) {
        lprintf("Mapping %p as page %d", addr, p);
        pt0[p] = (p << PAGE_SHIFT) | (1 << 10) | 3;
        addr += PAGE_SIZE;
        ++p;
    }

    pd[0] = ((uint32_t)pt0) | 1;

    lprintf("Transitioning into hyperspace...");
    hv_setpd(pd, 0);
    lprintf("YAY!!!");

    hv_exit(0);
}
