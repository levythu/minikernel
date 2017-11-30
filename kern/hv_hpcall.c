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
#include "zeus.h"

void initHyperCall() {
  int32_t* idtBase = (int32_t*)idt_base();
  idtBase[HV_INT  << 1] = ENCRYPT_IDT_TRAPGATE_LSB(
    3, (int32_t)(hyperCallHandler), 1, SEGSEL_KERNEL_CS, 1);
  idtBase[(HV_INT  << 1) + 1] = ENCRYPT_IDT_TRAPGATE_MSB(
    3, (int32_t)(hyperCallHandler), 1, SEGSEL_KERNEL_CS, 1);
}

// The dispatcher
int hyperCallHandler_Internal(int userEsp, int eax) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  if (!currentThread->process->hyperInfo.isHyper) {
    // Not a hypervisor. Cannot issue hyper call
    return -1;
  }

  HPC_ON(HV_MAGIC_OP, hpc_magic);
  HPC_ON(HV_EXIT_OP, hpc_exit);

  HPC_ON(HV_PRINT_OP, hpc_print);
  HPC_ON(HV_SET_COLOR_OP, hpc_cons_set_term_color);
  HPC_ON(HV_SET_CURSOR_OP, hpc_cons_set_cursor_pos);
  HPC_ON(HV_GET_CURSOR_OP, hpc_cons_get_cursor_pos);
  HPC_ON(HV_PRINT_AT_OP, hpc_print_at);

  HPC_ON(HV_DISABLE_OP, hpc_disable_interrupts);
  HPC_ON(HV_ENABLE_OP, hpc_enable_interrupts);
  HPC_ON(HV_SETIDT_OP, hpc_setidt);
  HPC_ON(HV_IRET_OP, hpc_iret);

  return -1;
}
