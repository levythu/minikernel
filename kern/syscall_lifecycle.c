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
#include <string.h>
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
  if (!pkg) {
    panic("exec_Internal: no kernel space");
  }

  // Verify and load execName
  int execName;
  if (!parseMultiParam(params, 0, &execName)) {
    // the pointer syscall package itself is invalid
    sfree(pkg, sizeof(ArgPackage));
    return -1;
  }
  int execLen = sGetString((uint32_t)execName, pkg->c[0], ARGPKG_MAX_ARG_LEN);
  if (execLen == ARGPKG_MAX_ARG_LEN || execLen == -1) {
    // the string is too long, or is invalid
    sfree(pkg, sizeof(ArgPackage));
    return -1;
  }

  // Verify and load argvec
  int argvec;
  if (!parseMultiParam(params, 1, &argvec)) {
    // the pointer syscall package itself is invalid
    sfree(pkg, sizeof(ArgPackage));
    return -1;
  }
  int argvecLen = sGeUIntArray(
      (uint32_t)argvec, NULL, ARGPKG_MAX_ARG_COUNT - 1);
  if (argvecLen == ARGPKG_MAX_ARG_COUNT - 1 || argvecLen == -1) {
    // too many args or invalid array, abort
    sfree(pkg, sizeof(ArgPackage));
    return -1;
  }
  // Fetch every string to arg pakage
  for (int i = 0; i < argvecLen; i++) {
    memset(pkg->c[i+1], 0, ARGPKG_MAX_ARG_LEN);
    int argstrLen = sGetString(
        ((uint32_t*)(argvec))[i] , pkg->c[i+1], ARGPKG_MAX_ARG_LEN);
    if (argstrLen == ARGPKG_MAX_ARG_LEN || argstrLen == -1) {
      // some argument is too long or invalid
      sfree(pkg, sizeof(ArgPackage));
      return -1;
    }
  }
  // Add terminator
  pkg->c[argvecLen+1][0] = 0;
  pkg->c[argvecLen+1][1] = -1;

  printArgPackage(pkg);
  sfree(pkg, sizeof(ArgPackage));
  return -1;
}
