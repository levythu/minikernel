/** @file syscall_lifecycle.c
 *
 *  @brief Syscalls about lifecycle
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

int task_vanish_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  lprintf("WARNING: Dummy task vanish is called by thread #%d of proc #%d. "
          "We just spin",
          currentThread->id, currentThread->process->id);
  while (true)
    ;
  return -1;
}

int gettid_Internal(SyscallParams params) {
  return getLocalCPU()->runningTID;
}

int halt_Internal(SyscallParams params) {
  // Good bye!
  sim_halt();
  panic("The OS should've been ceased.");
  return -1;
}

int fork_Internal(SyscallParams params) {
  // We own currentThread
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  // Precheck: the process has only one thread
  kmutexRLock(&currentThread->process->mutex);
  if (currentThread->process->numThread > 1) {
    kmutexRUnlock(&currentThread->process->mutex);
    // We reject a multithread process to fork
    return -1;
  }
  kmutexRUnlock(&currentThread->process->mutex);
  return forkProcess(currentThread);
}

int wait_Internal(SyscallParams params) {
  // We own currentThread
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);

  uint32_t statusPtr;
  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseSingleParam(params, (int*)(&statusPtr))) {
    // invalid ptr
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  if (statusPtr != 0 &&
      !verifyUserSpaceAddr(statusPtr, statusPtr + sizeof(int) - 1, true)) {
    // the address is not writable
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  int retAddr;
  int zombieTID = waitThread(currentThread, &retAddr);

  if (zombieTID >= 0 && statusPtr != 0) {
    // We need to double check, we dont know what happened when we sleep
    kmutexWLockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    if (!verifyUserSpaceAddr(statusPtr, statusPtr + sizeof(int) - 1, true)) {
      // the address is not writable
      kmutexWUnlockRecord(&currentThread->process->memlock,
          &currentThread->memLockStatus);
      return -1;
    }
    *((int*)statusPtr) = retAddr;
    kmutexWUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
  }

  return zombieTID;
}

int vanish_Internal(SyscallParams params) {
  // We own currentThread
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  // one way trip
  terminateThread(currentThread);
  return 0;
}

int set_status_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);

  int status;
  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseSingleParam(params, (int*)(&status))) {
    // invalid ptr
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  kmutexWLock(&currentThread->process->mutex);
  currentThread->process->retStatus = status;
  kmutexWUnlock(&currentThread->process->mutex);
  return 0;
}

int exec_Internal(SyscallParams params) {
  // We own currentThread
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  // Precheck: the process has only one thread
  kmutexRLock(&currentThread->process->mutex);
  if (currentThread->process->numThread > 1) {
    kmutexRUnlock(&currentThread->process->mutex);
    // We reject a multithread process to fork
    return -1;
  }
  kmutexRUnlock(&currentThread->process->mutex);

  // Then we needn't the lock, since we know that the process only have this
  // thread

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
  if (argvecLen == ARGPKG_MAX_ARG_COUNT - 1 || argvecLen == -1 ||
      argvecLen == 0) {
    // too many args or invalid array or no args (even the 1st one), abort
    sfree(pkg, sizeof(ArgPackage));
    return -1;
  }
  // Fetch every string to arg pakage, skip the first one
  for (int i = 1; i < argvecLen; i++) {
    memset(pkg->c[i], 0, ARGPKG_MAX_ARG_LEN);
    int argstrLen = sGetString(
        ((uint32_t*)(argvec))[i] , pkg->c[i], ARGPKG_MAX_ARG_LEN);
    if (argstrLen == ARGPKG_MAX_ARG_LEN || argstrLen == -1) {
      // some argument is too long or invalid
      sfree(pkg, sizeof(ArgPackage));
      return -1;
    }
  }
  // Add terminator
  pkg->c[argvecLen][0] = 0;
  pkg->c[argvecLen][1] = -1;

  printArgPackage(pkg);

  execProcess(currentThread, pkg->c[0], pkg);

  // When we reach here, execProcess fail.
  sfree(pkg, sizeof(ArgPackage));
  return -1;
}
