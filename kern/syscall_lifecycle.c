/** @file syscall.c
 *
 *  @brief TODO

 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <syscall_int.h>

#include "x86/asm.h"
#include "x86/cr.h"
#include "x86/seg.h"
#include "common_kern.h"
#include "syscall.h"
#include "process.h"
#include "int_handler.h"
#include "bool.h"
#include "cpu.h"
#include "zeus.h"

int gettid_Internal(SyscallParams params) {
  return getLocalCPU()->runningTID;
}

int fork_Internal(SyscallParams params) {
  // We own currentThread
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  // Precheck: the process has only one thread
  // TODO Consider locking the process data structure
  if (currentThread->process->numThread > 1) {
    // We reject a multithread process to fork
    return -1;
  }
  return forkProcess(currentThread);
}

int wait_Internal(SyscallParams params) {
  // TODO dummy
  lprintf("Dummy wait() is called. It loops");
  while (true)
    ;
}

int vanish_Internal(SyscallParams params) {
  // TODO dummy
  lprintf("Dummy vanish() is called. It loops");
  while (true)
    ;
}

int set_status_Internal(SyscallParams params) {
  return 0;
}
