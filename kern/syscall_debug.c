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
#include "pm.h"
#include "source_untrusted.h"
#include "kmutex.h"
#include "console.h"
#include "keyboard_event.h"
#include "page.h"

#define MISBEHAVE_NUM_SHOW_EVERYTHING 701

extern void reportKernelMemAlloc();
void dumpAll() {
  LocalLockR();
  lprintf("LevyOS Kernel Status──────────────────────────────────");
  reportUserMem();
  reportProcessAndThread();
  reportCPU();
  reportKernelMemAlloc();
  lprintf("└─────────────────────────────────────────────────────");
  LocalUnlockR();
}

int misbehave_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  int num;
  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseSingleParam(params, &num)) {
    // Invalid num
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  if (num == MISBEHAVE_NUM_SHOW_EVERYTHING) {
    dumpAll();
  }


  return 0;
}
