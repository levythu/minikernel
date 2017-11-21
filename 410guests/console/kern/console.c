/** @file console.c
 *
 *  @brief Exercises the hypervisor console
 *
 *  @author relong
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

static const int MAGIC_ROW = 5;
static const int MAGIC_COL = 10;

#define STRLEN(s) (sizeof(s) - 1)
static const char IN_HV[]    = "We are in the hypervisor\n";
static const char COLORS[]   = "Look at the pretty colors!\n";
static const char PASSING[]  = "All tests passing\n";
static const char TEST[]     = "This is a test";
static const char CLEARING[] = "Clearing screen\n\n\n\n\n\n";

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    if (!hv_isguest()) {
        lprintf("I am NOT supposed to be here!!!");
        panic("This payload is guest-only");
        return (-1);
    }

    lprintf("In guest kernel");

    int magic = hv_magic();
    lprintf("Found magic: %d", magic);
    if (magic != HV_MAGIC) {
        lprintf("Magic Failed");
        hv_exit(-10);
    }

    lprintf("Trying to print to the hypervisor");

    hv_print(STRLEN(IN_HV), (unsigned char *)IN_HV);


    lprintf("Setting hypervisor color");
    hv_cons_set_term_color(FGND_PINK | BGND_LGRAY);

    hv_print(STRLEN(COLORS), (unsigned char *)COLORS);

    lprintf("Moving cursor to 5:10");
    hv_cons_set_cursor_pos(MAGIC_ROW, MAGIC_COL);

    lprintf("Trying to get the cursor information now");
    int row, col;
    hv_cons_get_cursor_pos(&row, &col);

    lprintf("Found row, col: (%d, %d)", row, col);

    if (row != MAGIC_ROW || col != MAGIC_COL) {
        lprintf("Get Cursor Failed: (%d, %d)", row, col);
        hv_exit(-11);
    }

    hv_print_at(STRLEN(TEST),
                (unsigned char *)TEST,
                MAGIC_ROW + 1,  // Add some extra magic
                2 * MAGIC_COL,  // Add some extra magic
                FGND_CYAN | BGND_LGRAY);

    int new_row, new_col;
    hv_cons_get_cursor_pos(&new_row, &new_col);

    if (new_row != row || new_col != col) {
        lprintf("hv_print_at() changes state!!!");
        hv_exit(-12);
    }

    hv_print(STRLEN(PASSING), (unsigned char *)PASSING);

    lprintf("ALL TESTS PASSED");

    hv_print(STRLEN(CLEARING), (unsigned char *)CLEARING);

    hv_exit(77);

    panic("Exit failed?");
}
