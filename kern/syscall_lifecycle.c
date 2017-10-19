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
#include <stdlib.h>

#include "common_kern.h"
#include "syscall.h"
#include "process.h"
#include "int_handler.h"
#include "bool.h"
#include "cpu.h"
#include "zeus.h"
#include "loader.h"
#include "source_untrusted.h"

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

int exec_Internal(SyscallParams params) {
  ArgPackage* pkg = (ArgPackage*)smalloc(sizeof(ArgPackage));
  int execName;
  parseMultiParam(params, 0, &execName);
  int get = sGetString((uint32_t)execName, pkg->c[0], ARGPKG_MAX_ARG_LEN);
  lprintf("exec: %d:%s", get, pkg->c[0]);
  sfree(pkg, sizeof(ArgPackage));
  return -1;
}
