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
int hyperCallHandler_Internal(int userEsp, int eax,
    const int es, const int ds,
    const int _edi, const int _esi, const int _ebp, // pusha region
    const int _espOnCurrentStack, // pusha region
    const int _ebx, const int _edx, const int _, const int _eax, // pusha
    const int _ecx,
    const int eip, const int cs,  // from-user-mode only
    const int eflags, const int esp,  // from-user-mode only
    const int ss  // from-user-mode only
    ) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  if (!currentThread->process->hyperInfo.isHyper) {
    // Not a hypervisor. Cannot issue hyper call
    return -1;
  }
  assert(es == SEGSEL_GUEST_DS);
  assert(cs == SEGSEL_GUEST_CS);

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
  HPC_ON_X(HV_IRET_OP, hpc_iret);

  HPC_ON(HV_SETPD_OP, hpc_setpd);
  HPC_ON(HV_ADJUSTPG_OP, hpc_adjustpg);

  return -1;
}
