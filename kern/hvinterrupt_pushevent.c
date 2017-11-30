#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <hvcall_int.h>
#include <hvcall.h>
#include <x86/asm.h>
#include <x86/idt.h>
#include <x86/timer_defines.h>

#include "int_handler.h"
#include "common_kern.h"
#include "bool.h"
#include "process.h"
#include "cpu.h"
#include "hv.h"
#include "hvlife.h"
#include "source_untrusted.h"
#include "hv_hpcall_internal.h"
#include "hvinterrupt.h"
#include "hvinterrupt_pushevent.h"
#include "zeus.h"
#include "fault.h"

// In hvinterrupt.c, internal use only
bool applyInt(HyperInfo* info, hvInt hvi,
    uint32_t oldESP, uint32_t oldEFLAGS, uint32_t oldEIP);

void hv_CallMeOnTick(HyperInfo* info) {
  if (!HYPER_STATUS_READY(info->status)) return;

  uint32_t currentTic = __sync_fetch_and_add(&info->status, 1);
  if (currentTic % GENERATE_TIC_EVERY_N_TIMESLICE != 0) return;

  hvInt tint;
  tint.intNum = TIMER_IDT_ENTRY;
  tint.spCode = 0;
  tint.cr2 = 0;
  broadcastIntTo(&info->selfMulti, tint);
}

FAULT_ACTION(HyperFaultHandler) {
  lprintf("Hypervisor exception.");
  printError(es, ds, edi, esi, ebp,
      ebx, edx, ecx, eax, faultNumber, errCode,
      eip, cs, eflags, esp, ss, cr2);

  tcb* thr = findTCB(getLocalCPU()->runningTID);
  assert(thr);
  assert(thr->process->hyperInfo.isHyper);

  hvInt hvi;
  hvi.intNum = faultNumber;
  hvi.spCode = errCode;
  hvi.cr2 = faultNumber == IDT_PF ? cr2 : 0;

  if (!applyInt(&thr->process->hyperInfo, hvi, esp, eflags, eip)) {
    // Fail to deliver this, because no idt is registered. Crash guest
    exitHyperWithStatus(&thr->process->hyperInfo, thr, GUEST_CRASH_STATUS);
  }

  return true;
}
