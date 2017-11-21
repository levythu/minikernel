/** @file vast.c
 *
 *  @brief Creates a large address space and explores it
 *
 *  Vast maps the bottom four megabytes of "physical memory"
 *  repeatedly into the entire 32-bit virtual address space.
 *  It then reads from every other page (there shouldn't be
 *  any faults), totaling up the values it reads, and then
 *  lprintf()s the nonsense value that results.
 *
 *  When complete, it hv_exit(99)
 *
 *  @author de0u
 *  @author relong
 *
 *  Hello is a PebPeb-only executable -- it doesn't work on hardware.
 *
 */

#include <common_kern.h>

/* libc includes. */
#include <malloc.h>
#include <simics.h> /* lprintf() */
#include <stdio.h>
#include <string.h>

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

#include <compiler.h>
#include <hvcall.h>
#include <malloc_internal.h>

#define STRLEN(s) (sizeof(s) - 1)
static const char READY_MESSAGE[]   = "Setting Page Directory\n";
static const char SET_MESSAGE[]     = "Page Directory Set successfully\n";
static const char PASSING_MESSAGE[] = "Vast passing\n";

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    uint32_t *pt, *pd, *p;
    int       i, t = 0;


    lprintf("Hello from a brand new kernel!");

    if (!hv_isguest()) {
        lprintf("I am NOT supposed to be here!!!");
        panic("This payload is guest-only");
        return (-1);
    }

    pt = _smemalign(PAGE_SIZE, PAGE_SIZE);
    pd = _smemalign(PAGE_SIZE, PAGE_SIZE);
    if (!pt || !pd) {
        panic("Cannot allocate memory");
    }

    for (i = 0; i < (PAGE_SIZE / sizeof(uint32_t)); i++) {
        pt[i] = (i << PAGE_SHIFT) | 3;
        pd[i] = ((uint32_t)pt) | 3;
    }

    // Unmap the last 16 MB
    for (i = 0; i < 4; i++) {
        pd[(PAGE_SIZE / sizeof(uint32_t)) - 1 - i] = 0;
    }

    lprintf("I hope this works...");
    hv_print(STRLEN(READY_MESSAGE), (unsigned char *)READY_MESSAGE);
    hv_setpd(pd, 0);
    lprintf("Whew!!  Now let's take a look around.");
    hv_print(STRLEN(SET_MESSAGE), (unsigned char *)SET_MESSAGE);
    lprintf("Reading now");

    void *maxAddress = hv_maxvaddr();

    lprintf("Max Address: %p", maxAddress);

    for (p = (uint32_t *)0; p <= (uint32_t *)maxAddress; p += 8192) {
        t += *p;
    }

    lprintf("YAY!!! (t = 0x%x, FWIW)", t);
    hv_print(STRLEN(PASSING_MESSAGE), (unsigned char *)PASSING_MESSAGE);

    hv_exit(99);

    while (1) {
        lprintf("In dead man loop");
        continue;
    }

    return -1;
}
