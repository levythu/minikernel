#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <page.h>
#include <x86/asm.h>

#include "common_kern.h"
#include "bool.h"
#include "hv.h"
#include "mode_switch.h"
#include "hvcall.h"
#include "dbgconf.h"
#include "hvinterrupt.h"
#include "hvinterrupt_timer.h"

MAKE_VAR_QUEUE_UTILITY(hvInt);

bool fillHyperInfo(simple_elf_t* elfMetadata, HyperInfo* info) {
  if (elfMetadata->e_txtstart < USER_MEM_START) {
    // We are virtual machine!

    // 1. Apply the offset to elf
    elfMetadata->e_entry += GUEST_PHYSICAL_START;
    elfMetadata->e_txtstart += GUEST_PHYSICAL_START;
    elfMetadata->e_datstart += GUEST_PHYSICAL_START;
    elfMetadata->e_rodatstart += GUEST_PHYSICAL_START;
    elfMetadata->e_bssstart += GUEST_PHYSICAL_START;

    // 2. Write HyperInfo
    info->cs = SEGSEL_GUEST_CS;
    info->ds = SEGSEL_GUEST_DS;
    info->baseAddr = GUEST_PHYSICAL_START;
    info->isHyper = true;
  } else {
    // normal elf

    info->cs = SEGSEL_USER_CS;
    info->ds = SEGSEL_USER_DS;
    info->baseAddr = 0;
    info->isHyper = false;
  }

  return info->isHyper;
}

void destroyHyperInfo(HyperInfo* info) {
  removeWaiter(&timeMultiplexter, info);

  varQueueDestroy(&info->delayedInt);
  sfree(info->idt, sizeof(IDTEntry) * (MAX_SUPPORTED_VIRTUAL_INT + 1));
}

void bootstrapHypervisorAndSwitchToRing3(
    HyperInfo* info, uint32_t entryPoint, uint32_t eflags) {
  assert(info->isHyper);

  // set all the other fields for hyperInfo before activating it
  info->interrupt = false;
  initCrossCPULock(&info->latch);
  varQueueInit(&info->delayedInt, MAX_WAITING_INT);
  // TODO: check sucessful of smalloc
  info->idt = (IDTEntry*)smalloc(sizeof(IDTEntry) *
                                 (MAX_SUPPORTED_VIRTUAL_INT + 1));
  for (int i = 0; i <= MAX_SUPPORTED_VIRTUAL_INT; i++) {
    info->idt[i].present = false;
  }

  // register myself to timer multiplexer
  addToWaiter(&timeMultiplexter, info);

  #ifdef HYPERVISOR_VERBOSE_PRINT
    lprintf("Entering into virtual machine at 0x%08lx", entryPoint);
  #endif
  switchToRing3X(0, eflags, entryPoint - info->baseAddr, 0,
                 0, 0, HYPERVISOR_MEMORY / PAGE_SIZE, 0,
                 GUEST_PHYSICAL_MAXVADDR, GUEST_LAUNCH_EAX,
                 info->cs, info->ds);
}
