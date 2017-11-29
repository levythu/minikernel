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

int hpc_magic(int userEsp, tcb* thr) {
  return HV_MAGIC;
}

int hpc_exit(int userEsp, tcb* thr) {
  DEFINE_PARAM(int, status, 0);

  thr->process->retStatus = status;
  // one way trp
  terminateThread(thr);
  return -1;
}
