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
#include <x86/video_defines.h>

#include "common_kern.h"
#include "syscall.h"
#include "process.h"
#include "int_handler.h"
#include "bool.h"
#include "cpu.h"
#include "zeus.h"
#include "loader.h"
#include "source_untrusted.h"
#include "kmutex.h"
#include "console.h"
#include "keyboard_event.h"
#include "fault_handler_user.h"
#include "mode_switch.h"
#include "page.h"
#include "timeout.h"
#include "scheduler.h"
#include "context_switch.h"

int thread_fork_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  return forkThread(currentThread);
}

int make_runnable_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  int tid;

  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseSingleParam(params, &tid)) {
    // Invalid tid
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  tcb* targetThread = findTCBWithEphemeralAccess(tid);
  if (!targetThread) {
    return -1;
  }
  // TODO: the target thread may not even finish init!, so dmlock is not
  // prepared yet
  bool succ = false;
  GlobalLockR(&targetThread->dmlock);
  if (targetThread->status == THREAD_BLOCKED_USER) {
    targetThread->status = THREAD_RUNNABLE;
    succ = true;
  }
  GlobalUnlockR(&targetThread->dmlock);

  if (succ) {
    // try to own target and switch
    LocalLockR();
    bool owned = __sync_bool_compare_and_swap(
        &targetThread->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD);
    releaseEphemeralAccess(targetThread);
    if (!owned) {
      // someone else is scheduling, nvm
      LocalUnlockR();
      return 0;
    }
    swtichToThread_Prelocked(targetThread);
    return 0;
  }
  releaseEphemeralAccess(targetThread);
  return -1;
}

int deschedule_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  uint32_t rejectAddr;

  // Use wlock to exclude make runnable
  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseSingleParam(params, (int*)&rejectAddr)) {
    // Invalid rejectAddr
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  // TODO: check the other verifyUserSpaceAddr calls about size
  if (!verifyUserSpaceAddr(rejectAddr, rejectAddr + sizeof(int) - 1, false)) {
    // the address is not valid
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }

  GlobalLockR(&currentThread->dmlock);
  bool goingAway = false;
  if (*((int*)rejectAddr) == 0) {
    currentThread->status = THREAD_BLOCKED_USER;
    goingAway = true;
  }
  GlobalUnlockR(&currentThread->dmlock);

  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  if (goingAway) {
    yieldToNext();
  }
  return 0;
}

int yield_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  int tid;

  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseSingleParam(params, &tid)) {
    // Invalid tid
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  if (tid == -1) {
    // pick the next to run
    yieldToNext();
    return 0;
  }
  tcb* targetThread = findTCBWithEphemeralAccess(tid);
  if (!targetThread) {
    return -1;
  }
  if (!THREAD_STATUS_CAN_RUN(targetThread->status)) {
    releaseEphemeralAccess(targetThread);
    return -1;
  }
  // Try to run it by acquiring the lock. If can acquire, it's fine. Otherwise
  // someone else is trying to run it, just return succ
  LocalLockR();
  bool owned = __sync_bool_compare_and_swap(
      &targetThread->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD);
  releaseEphemeralAccess(targetThread);
  if (!owned) {
    LocalUnlockR();
    return 0;
  }
  swtichToThread_Prelocked(targetThread);
  return 0;
}

int sleep_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  int ticks;

  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseSingleParam(params, &ticks)) {
    // Invalid ticks
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  if (ticks < 0) return -1;
  if (ticks == 0) return 0;
  sleepFor(ticks);
  return 0;
}

int swexn_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  uint32_t esp3, eip, uregAddr;
  int userArg;

  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  // Validate & get esp3
  if (!parseMultiParam(params, 0, (int*)(&esp3))) {
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    // invalid esp3 address
    return -1;
  }
  // Validate & get eip
  if (!parseMultiParam(params, 1, (int*)(&eip))) {
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    // invalid eip address
    return -1;
  }
  // Co-validate esp3 and eip
  if (esp3 != 0 && eip != 0) {
    // Register handler mode, validate address
    if (!verifyUserSpaceAddr(esp3 - 2 - SWEXN_minimalStackSize,
                             esp3 - 2, true)) {
      kmutexRUnlockRecord(&currentThread->process->memlock,
          &currentThread->memLockStatus);
      // invalid esp3
      return -1;
    }
    if (!verifyUserSpaceAddr(eip, eip, false)) {
      kmutexRUnlockRecord(&currentThread->process->memlock,
          &currentThread->memLockStatus);
      // invalid eip
      return -1;
    }
  }
  // Validate & get arg
  if (!parseMultiParam(params, 2, (int*)(&userArg))) {
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    // invalid eip address
    return -1;
  }

  // validate & get uregAddr
  if (!parseMultiParam(params, 3, (int*)(&uregAddr))) {
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    // invalid eip address
    return -1;
  }
  // get ureg
  ureg_t userUreg;
  if (uregAddr != 0) {
    if (!verifyUserSpaceAddr(uregAddr, uregAddr + sizeof(ureg_t) -1, false)) {
      kmutexRUnlockRecord(&currentThread->process->memlock,
          &currentThread->memLockStatus);
      // invalid ureg
      return -1;
    }
    userUreg = *(ureg_t*)uregAddr;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  if (uregAddr != 0 && !validateUregs(&userUreg)) {
    // invalid ureg
    return -1;
  }
  // Do things!
  if (esp3 != 0 && eip != 0) {
    // Register handler mode
    registerUserFaultHandler(currentThread, esp3, eip, userArg);
  } else {
    deregisterUserFaultHandler(currentThread);
  }
  if (uregAddr != 0) {
    // jump to target ureg
    switchToRing3X(userUreg.esp, userUreg.eflags, userUreg.eip, userUreg.edi,
                   userUreg.esi, userUreg.ebp,    userUreg.ebx, userUreg.edx,
                   userUreg.ecx, userUreg.eax);
  }
  return 0;
}
