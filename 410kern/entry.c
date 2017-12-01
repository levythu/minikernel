/** @file boot/entry.c
 *  @brief Entry point for the C part of the kernel
 *  @author matthewj S2008
 *  @author elly1 S2010
 */

#include <common_kern.h>
#include <x86/cr.h>
#include <x86/page.h>
#include <x86/interrupt_defines.h>
#include <boot/multiboot.h>
#include <boot/util.h>
#include <simics.h>
#include <lmm/lmm.h>
#include <malloc/malloc_internal.h>
#include <assert.h>

#include <kvmphys.h>

/* For FLUX compatibility; see lib/kvmphys.h */
vm_offset_t phys_mem_va = 0;

/* 410 interface compatibility */
static int n_phys_frames = 0;

/** @brief C entry point */
void mb_entry(mbinfo_t *info, void *istack) {
    int argc;
    char **argv;
    char **envp;

    /* Make sure mem_upper has been set by boot loader */
    assert(info->flags & MULTIBOOT_MEMORY);

    /* Want (kilobytes*1024)/PAGE_SIZE, but definitely avoid overflow */
    n_phys_frames = (info->mem_upper+1024)/(PAGE_SIZE/1024);
    assert(n_phys_frames > (USER_MEM_START/PAGE_SIZE));

    // LMM: init malloc_lmm and reserve memory holding this executable
    mb_util_lmm(info, &malloc_lmm);
    // LMM: don't give out memory under 1 megabyte
    lmm_remove_free(&malloc_lmm, (void*)0, 0x100000);
    // LMM: don't give out memory between USER_MEM_START and infinity
    lmm_remove_free(&malloc_lmm, (void*)USER_MEM_START, -8 - USER_MEM_START);

    // lmm_dump(&malloc_lmm);

    mb_util_cmdline(info, &argc, &argv, &envp);

    // Having done that, let's tell Simics we've booted.
    sim_booted(argv[0]);

    /* Disable floating-point unit:
     * inadvisable for kernel, requires context-switch code for users
     */
    set_cr0(get_cr0() | CR0_EM);

    /* Initialize the PIC so that IRQs use different IDT slots than
     * CPU-defined exceptions.
     */
    interrupt_setup();

    kernel_main(info, argc, argv, envp);
}

#include <lmm/lmm_types.h>

static int hv_guest_mode = 0;

static unsigned int hv_max_vaddr = 0;

/** @brief guest-mode C entry point */
void hv_guest_entry(unsigned int maxframe, unsigned int maxvaddr) {
    char *argv[] = { "kernel", "-guest", 0} ;

    n_phys_frames = maxframe - 4 + 1; /* XXX */
    assert(n_phys_frames > (USER_MEM_START/PAGE_SIZE));

    // LMM init, short form
    static struct lmm_region region;
    extern char end[];
    lmm_init(&malloc_lmm);
    lmm_add_region(&malloc_lmm, &region, 0, USER_MEM_START, 0, 1); // kvm
    lmm_add_free(&malloc_lmm, end, (char *)USER_MEM_START - end); // available kvm

    hv_max_vaddr = maxvaddr;

    hv_guest_mode = 1;

    kernel_main(0, 2, argv, 0);
}

int hv_isguest(void)
{
    return (hv_guest_mode);
}

void *
hv_maxvaddr(void)
{
    if (!hv_guest_mode)
        panic("hv_maxvaddr() called in host mode");

    return ((void *)hv_max_vaddr);
}

int
machine_phys_frames(void)
{
    return (n_phys_frames);
}
