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
