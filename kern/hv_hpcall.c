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
#include "hv.h"
#include "hv_hpcall_internal.h"

void initHyperCall() {
  int32_t* idtBase = (int32_t*)idt_base();
  idtBase[HV_INT  << 1] = ENCRYPT_IDT_TRAPGATE_LSB(
    3, (int32_t)(hyperCallHandler), 1, SEGSEL_KERNEL_CS, 1);
  idtBase[(HV_INT  << 1) + 1] = ENCRYPT_IDT_TRAPGATE_MSB(
    3, (int32_t)(hyperCallHandler), 1, SEGSEL_KERNEL_CS, 1);
}

static int hpc_magic(int userEsp);

// The dispatcher
int hyperCallHandler_Internal(int userEsp, int eax) {
  HPC_ON(HV_MAGIC_OP, hpc_magic);
  return -1;
}

/******************************************************************************/

static int hpc_magic(int userEsp) {
  return HV_MAGIC;
}
