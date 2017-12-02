/** @file hvlife.c
 *
 *  @brief Controlling the life time for a hypervisor
 *
 *
 *  @author Leiyu Zhao
 */

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
#include "hvinterrupt_pushevent.h"
#include "keyboard_event.h"
#include "process.h"
#include "zeus.h"
#include "hvvm.h"

MAKE_VAR_QUEUE_UTILITY(hvInt);

static void initHyperInfo(HyperInfo* info) {
  info->cs = SEGSEL_USER_CS;
  info->ds = SEGSEL_USER_DS;
  info->baseAddr = 0;
  info->isHyper = false;
  info->status = HyperNA;
}

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
    info->status = HyperNew;
  } else {
    // normal elf

    info->cs = SEGSEL_USER_CS;
    info->ds = SEGSEL_USER_DS;
    info->baseAddr = 0;
    info->isHyper = false;
    info->status = HyperNA;
  }

  return info->isHyper;
}

void destroyHyperInfo(HyperInfo* info, tcb* thr, int vcn) {
  removeWaiter(&info->selfMulti, info);
  intMultiplexer* currentKB = getKeyboardMultiplexer(vcn);
  removeWaiter(currentKB, info);

  varQueueDestroy(&info->delayedInt);
  sfree(info->idt, sizeof(IDTEntry) * (MAX_SUPPORTED_VIRTUAL_INT + 1));

  exitPagingMode(thr);
}

void exitHyperWithStatus(HyperInfo* info, void* _thr, int statusCode) {
  tcb* thr = (tcb*)_thr;
  assert((&thr->process->hyperInfo) == info);
  destroyHyperInfo(info, thr, thr->process->vcNumber);

  thr->process->retStatus = statusCode;
  // one way trp
  terminateThread(thr);
}

void bootstrapHypervisorAndSwitchToRing3(
    HyperInfo* info, uint32_t entryPoint, uint32_t eflags, int vcn) {
  assert(info->isHyper);

  // 1. set all the other fields for hyperInfo before activating it
  info->interrupt = false;

  info->tics = 0;

  info->originalPD = NULL;
  info->vCR3 = 0;
  info->writeProtection = false;
  info->inKernelMode = true;
  info->esp0 = 0;

  initMultiplexer(&info->selfMulti);

  initCrossCPULock(&info->latch);
  varQueueInit(&info->delayedInt, MAX_WAITING_INT);
  info->idt = (IDTEntry*)smalloc(sizeof(IDTEntry) *
                                 (MAX_SUPPORTED_VIRTUAL_INT + 1));
  if (!info->idt) {
    panic("No enough kernel memory to launch hypervisor");
  }
  for (int i = 0; i <= MAX_SUPPORTED_VIRTUAL_INT; i++) {
    info->idt[i].present = false;
  }

  // 2. register myself to self-multiplexer
  addToWaiter(&info->selfMulti, info);
  // listen to keyboard
  intMultiplexer* currentKB = getKeyboardMultiplexer(vcn);
  addToWaiter(currentKB, info);

  info->status = HyperInited;

  #ifdef HYPERVISOR_VERBOSE_PRINT
    lprintf("Entering into virtual machine at 0x%08lx", entryPoint);
  #endif
  switchToRing3X(0, eflags, entryPoint - info->baseAddr, 0,
                 0, 0, HYPERVISOR_MEMORY / PAGE_SIZE, 0,
                 GUEST_PHYSICAL_MAXVADDR, GUEST_LAUNCH_EAX,
                 info->cs, info->ds);
}
