/** @file fondle2.c
 *
 *  @brief A simple test of the hypervisor's virtual memory system.
 *
 *  This test exercises hv_setpd(), hv_adjustpg(), and page-fault delivery.
 *
 *  The test proceeds as follows.
 *
 *  1. Insert a static mapping which direct maps the bottom 16MB for user
 *     and kernel space.
 *
 *  2. A special 4MB region of the address space is mapped for kernel space
 *     only.
 *
 *  3. In kernel mode the numbers 0 through 1023 are written to the pages
 *     at the top of memory.
 *
 *  4. The top page table is reversed and hv_adjustpg() is called on all
 *     of the addresses in the top 4MB.
 *
 *  5. Values are checked to make sure they reversed along with the frames.
 *
 *  6. hv_iret goes to "user" mode and expects a page fault when it tries
 *     to touch the top memory.
 *
 *  @author mischnal
 *  @author relong
 *
 *  PebPeb-only executable -- it doesn't work on hardware.
 *
 */

#include <common_kern.h>

/* libc includes. */
#include <malloc.h>
#include <simics.h> /* lprintf() */
#include <stdint.h>
#include <stdio.h>

/* multiboot header file */
#include <multiboot.h> /* boot_info */

/* memory includes. */
#include <lmm.h> /* lmm_remove_free() */

/* x86 specific includes */
#include <eflags.h>
#include <x86/asm.h> /* enable_interrupts() */
#include <x86/cr.h>
#include <x86/interrupt_defines.h> /* interrupt_setup() */
#include <x86/page.h>
#include <x86/seg.h> /* install_user_segs() */

#include <hvcall.h>
#include <malloc_internal.h>

#include <assert.h>
#include <string.h>

#define KERNEL_PAGE_DIRECTORY_INDEX 410
#define STACK_SIZE 1024

static const uint32_t MB             = 1024 * 1024;
static const uint32_t KERNEL_ADDRESS = KERNEL_PAGE_DIRECTORY_INDEX << 22;
static const uint32_t MAGIC_OFFSET   = 0x9f1;


static int tests_passed = 0;

static uint32_t user_stack[STACK_SIZE];
static uint32_t kernel_stack[STACK_SIZE];


static void page_fault(void);
static void tick(void);
static void write_test(void);
static void rev_array(void* arrv, size_t elm_size, size_t array_size);
static void read_test(void);

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t* mbinfo, int argc, char** argv, char** envp) {
    lprintf("Well, here I am!");

    if (!hv_isguest()) {
        lprintf("Not a guest?");
        panic("This payload is guest-only");
        return -1;
    }

    lprintf("Starting fondle");
    // Setup page table
    // Bottom 16MB direct mapped for both kernel and user access
    uint32_t* pageDirectory = _smemalign(PAGE_SIZE, PAGE_SIZE);
    uint32_t* directMapPageTables =
        _smemalign(PAGE_SIZE, PAGE_SIZE * sizeof(uint32_t));
    for (int pageTableIndex = 0; pageTableIndex < PAGE_SIZE; pageTableIndex++) {
        uint32_t guestFrame                 = pageTableIndex * PAGE_SIZE;
        directMapPageTables[pageTableIndex] = guestFrame | 7;  // UWP
    }
    for (int pageDirectoryIndex = 0; pageDirectoryIndex < 4;
         pageDirectoryIndex++) {
        pageDirectory[pageDirectoryIndex] =
            (uint32_t)&directMapPageTables[pageDirectoryIndex
                                           * (PAGE_SIZE / sizeof(uint32_t))]
            | 7;  // UWP
    }
    // 4MB special region mapped for kernel only
    uint32_t* kernelPageTable = _smemalign(PAGE_SIZE, PAGE_SIZE);
    for (int pageTableIndex = 0;
         pageTableIndex < (PAGE_SIZE / sizeof(uint32_t));
         pageTableIndex++) {
        uint32_t guestFrame = USER_MEM_START + (pageTableIndex * PAGE_SIZE);
        kernelPageTable[pageTableIndex] = guestFrame | 3;  // KWP
    }
    pageDirectory[KERNEL_PAGE_DIRECTORY_INDEX] =
        (uint32_t)kernelPageTable | 3;  // KWP

    lprintf("Setting up page directory");
    hv_setpd(pageDirectory, 0);

    lprintf("Setting up IDT");

    hv_setidt(14, page_fault, 1);
    hv_setidt(32, tick, 1);
    hv_setidt(33, tick, 1);

    write_test();

    rev_array(kernelPageTable, sizeof(uint32_t), PAGE_SIZE / sizeof(uint32_t));

    for (uint32_t guest_virtual = KERNEL_ADDRESS;
         guest_virtual < (KERNEL_ADDRESS + 4 * MB);
         guest_virtual += PAGE_SIZE) {
        hv_adjustpg((void*)guest_virtual);
    }

    read_test();

    // Go to user mode
    hv_iret(read_test,
            get_eflags() & ~EFL_IF,  // Force interrupts to be disabled
            user_stack + STACK_SIZE - 1,
            kernel_stack + STACK_SIZE - 1,
            0);

    lprintf("HV iret returned");

    hv_exit(-12);

    while (1) {
        continue;
    }
}

static void write_test(void) {
    int i = 0;
    // Set values 0 - 1023
    for (uint32_t guest_virtual = KERNEL_ADDRESS;
         guest_virtual < (KERNEL_ADDRESS + 4 * MB);
         guest_virtual += PAGE_SIZE) {
        *(int*)(guest_virtual + MAGIC_OFFSET) = i++;
    }
    i = 0;
    for (uint32_t guest_virtual = KERNEL_ADDRESS;
         guest_virtual < (KERNEL_ADDRESS + 4 * MB);
         guest_virtual += PAGE_SIZE) {
        int i_prime = *(int*)(guest_virtual + MAGIC_OFFSET);
        if (i != i_prime) {
            lprintf("Value written to page %lx did not appear (%d != %d)",
                    guest_virtual,
                    i,
                    i_prime);
            hv_exit(-11);
        }
        i++;
    }
    lprintf("fondle: Write test complete");
    tests_passed++;
}

static void read_test(void) {
    int i = (PAGE_SIZE / sizeof(uint32_t)) - 1;

    for (uint32_t guest_virtual = KERNEL_ADDRESS;
         guest_virtual < (KERNEL_ADDRESS + 4 * MB);
         guest_virtual += PAGE_SIZE) {
        int i_prime = *(int*)(guest_virtual + MAGIC_OFFSET);
        if (i != i_prime) {
            lprintf("Value written to page %lx did not appear (%d != %d)",
                    guest_virtual,
                    i,
                    i_prime);
            hv_exit(-11);
        }
        i--;
    }
    lprintf("fondle: Read test completed");
    tests_passed++;
}

static void page_fault(void) {
    lprintf("In page fault handler");

    if (tests_passed == 2) {
        lprintf("Fondle passed");
        hv_exit(1);
    } else {
        lprintf("Fondle failed (early page fault)");
        hv_exit(-13);
    }
}

static void tick(void) {
    // Interrupts never enabled
    lprintf("Tick called?");
    hv_exit(-10);
}

static void rev_array(void* arrv, size_t elm_size, size_t array_size) {
    size_t beg, end;
    char*  arr = (char*)arrv;

    beg = 0;
    end = (elm_size * (array_size - 1));

    while (beg < end) {
        int i;
        for (i = 0; i < elm_size; i++) {
            char temp    = arr[beg + i];
            arr[beg + i] = arr[end + i];
            arr[end + i] = temp;
        }
        beg += elm_size;
        end -= elm_size;
    }
}
