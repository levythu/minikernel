/** @file station.c
 *
 *  @brief Exercises clock interrupts in a charming fashion.
 *
 *  @author de0u
 *
 *  PebPeb-only executable -- it doesn't work on hardware.
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
#include <video_defines.h>

#define DEBUG
#include <contracts.h>

#include <assert.h>
#include <string.h>

/* https://classictrainsongs.com/down_by_station/down_by_the_station.htm */
char *lyrics[] = {
    "Down by the station early in the morning,\n",
    "See the little puffer bellies all in a row.\n",
    "See the station master turn the little handle,\n", 
    "Chug, chug, toot, toot, off we go.\n",
    NULL
};

void
twiddle(void)
{
    volatile int accumulator = 0;
    while (accumulator != -1)
        ++accumulator;
    panic("Uh oh, overflow, population, common group");
}

#define DELAY_TICKS 10 // If you make this much larger, what happens next will shock you!

volatile int tick = 0;
volatile int line = 0;

void
tickback(void)
{
    if (++tick == DELAY_TICKS) {
        tick = 0;
        hv_print(strlen(lyrics[line]), (unsigned char *)lyrics[line]);
        ++line;
        if (lyrics[line] == NULL)
            hv_exit(0);
    }
    hv_enable_interrupts();
    twiddle();
}

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int
kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp)
{
    if (!hv_isguest()) {
        lprintf("I am NOT supposed to be here!!!");
        panic("This payload is guest-only");
        return (-1);
    }
    hv_setidt(HV_TICKBACK, tickback, HV_SETIDT_PRIVILEGED);
    hv_setidt(HV_KEYBOARD, tickback, HV_SETIDT_PRIVILEGED);
    hv_enable_interrupts();
    twiddle();
    return (-99);
}
