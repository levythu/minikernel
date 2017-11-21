/**
 * @brief A simple test of interrupt delivery
 *
 * Prints tock every 50 timer tick received and print tick every
 * time a key is hit
 *
 * Requires hv_iret going kernel to kernel, hv_print, and hv_setidt
 *
 * @author mischnal
 * @author relong
 *
 * PebPeb-only executable -- it doesn't work on hardware.
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
#include <x86/eflags.h>
#include <x86/interrupt_defines.h> /* interrupt_setup() */
#include <x86/keyhelp.h>
#include <x86/page.h>
#include <x86/seg.h> /* install_user_segs() */
#include <x86/timer_defines.h>

#include <hvcall.h>
#include <malloc_internal.h>

#include <assert.h>
#include <string.h>

static void tick(void);
static void tock(void);
static void loop(void);

static long stack[1024];

#define STRLEN(s) (sizeof(s) - 1)
static const char tick_message[] = "tick\n";
static const char tock_message[] = "tock\n";

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    lprintf("Well, here I am!");

    if (!hv_isguest()) {
        lprintf("Not a guest?");
        panic("This payload is guest-only");
        return -1;
    }

    lprintf("Successfully running as a guest");

    hv_setidt(KEY_IDT_ENTRY, tick, HV_SETIDT_PRIVILEGED);
    hv_setidt(TIMER_IDT_ENTRY, tock, HV_SETIDT_PRIVILEGED);

    hv_enable_interrupts();

    while (1) {
        continue;
    }
}

static void tick(void) {
    hv_print(STRLEN(tick_message), (unsigned char *)tick_message);
    hv_iret(loop,
            get_eflags(),  // Interrupts enabled
            (stack + 1023),
            0,
            0);
}

static void tock(void) {
    static unsigned int tocks = 0;
    tocks++;
    if (tocks % 50 == 0) {
        hv_print(STRLEN(tock_message), (unsigned char *)tock_message);
    }
    hv_iret(loop,
            get_eflags(),  // Interrupts enabled
            stack + 1023,
            0,
            0);
}

static void loop(void) {
    while (1) {
        continue;
    }
}
