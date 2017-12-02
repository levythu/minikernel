/** @file hv_hpcall_int.c
 *
 *  @brief Implementation of interrupt related hypercalls
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <hvcall_int.h>
#include <hvcall.h>
#include <x86/asm.h>
#include <x86/eflags.h>

#include "int_handler.h"
#include "common_kern.h"
#include "bool.h"
#include "process.h"
#include "cpu.h"
#include "hv.h"
#include "hvlife.h"
#include "source_untrusted.h"
#include "hv_hpcall_internal.h"
#include "mode_switch.h"
#include "hvinterrupt.h"
#include "zeus.h"
#include "hvvm.h"


int hpc_disable_interrupts(int userEsp, tcb* thr) {
  thr->process->hyperInfo.interrupt = false;
  return 0;
}

int hpc_enable_interrupts(int userEsp, tcb* thr) {
  thr->process->hyperInfo.interrupt = true;
  return 0;
}

int hpc_setidt(int userEsp, tcb* thr) {
  DEFINE_PARAM(int, irqno, 0);
  DEFINE_PARAM(uint32_t, eip, 1);
  DEFINE_PARAM(int, privileged, 2);

  if (irqno < 0 || irqno > MAX_SUPPORTED_VIRTUAL_INT) {
    lprintf("Warning:: non-conforming IDT setting, "
            "try to set irqno = %d. Ignored.", irqno);
    return -1;
  }

  // TODO: handle removal

  HyperInfo* info = &thr->process->hyperInfo;
  GlobalLockR(&info->latch);
  info->idt[irqno].present = true;
  info->idt[irqno].eip = eip;
  info->idt[irqno].privileged = privileged;
  GlobalUnlockR(&info->latch);
  return 0;
}

int hpc_iret(int userEsp, tcb* thr,
    int oedi, int oesi, int oebp, int oebx, int oedx, int oecx, int oeax) {
  DEFINE_PARAM(uint32_t, eip, 0);
  DEFINE_PARAM(uint32_t, eflags, 1);
  DEFINE_PARAM(uint32_t, esp, 2);
  DEFINE_PARAM(uint32_t, esp0, 3);
  DEFINE_PARAM(uint32_t, eax, 4);

  HyperInfo* info = &thr->process->hyperInfo;

  // Manually apply IF flag
  if (eflags & EFL_IF) {
    info->interrupt = true;
  } else {
    info->interrupt = false;
  }
  eflags |= EFL_IF;
  if (!validateEFLAGS(eflags)) {
    lprintf("Hypervisor crashes: bad eflags");
    exitHyperWithStatus(info, thr, GUEST_CRASH_STATUS);
    return -1;
  }

  if (esp0 != 0) {
    // Switch to guest ring3!!
    info->inKernelMode = false;
    if (!info->originalPD) {
      // Oh... how can you get into user mode without turning on paging !?
      lprintf("Hypervisor crashes: Try to switch to user mode with "
              "paging off");
      exitHyperWithStatus(info, thr, GUEST_CRASH_STATUS);
    }
    reActivateOriginalPD(thr);
    // Recompile PD to shadow kernel only memories
    if (!swtichGuestPD(thr)) {
      lprintf("Hypervisor crashes: fail to recompile user page table");
      exitHyperWithStatus(info, thr, GUEST_CRASH_STATUS);
    }
    info->esp0 = esp0;
  }

  // One-way trip
  switchToRing3X(esp, eflags, eip, oedi, oesi, oebp, oebx, oedx, oecx, eax,
                 info->cs, info->ds);
  return 0;
}
