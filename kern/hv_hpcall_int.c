#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <hvcall_int.h>
#include <hvcall.h>
#include <x86/asm.h>

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
    // TODO crash the guest
    return -1;
  }

  HyperInfo* info = &thr->process->hyperInfo;
  GlobalLockR(&info->latch);
  info->idt[irqno].present = true;
  info->idt[irqno].eip = eip;
  info->idt[irqno].privileged = privileged;
  GlobalUnlockR(&info->latch);
  return 0;
}
