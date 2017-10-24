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
#include "keyboard_event.h"

#define MAX_READLINE_BUFFER_SIZE (CONSOLE_WIDTH * CONSOLE_HEIGHT)
int readline_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);

  int len;
  uint32_t bufAddr;
  kmutexRLock(&currentThread->process->mutex);
  if (!parseMultiParam(params, 0, &len)) {
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  if (len > MAX_READLINE_BUFFER_SIZE) {
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  if (!parseMultiParam(params, 1, (int*)(&bufAddr))) {
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  if (!verifyUserSpaceAddr(bufAddr, bufAddr + len - 1, true)) {
    // The buffer itself is not writable or has problems
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  kmutexRUnlock(&currentThread->process->mutex);

  // allocate a kernekl mem to get the chars first, because we don't want
  // memlock to be held when waiting keyboard

  char* buf = smalloc(len);
  if (!buf) {
    // no kernel mem available,
    return -1;
  }

  occupyKeyboard();
  int actualLen = getStringBlocking(buf, len);
  releaseKeyboard();

  kmutexRLock(&currentThread->process->mutex);
  // Revalidate, the page table may be removed when waiting for keyboard
  if (!verifyUserSpaceAddr(bufAddr, bufAddr + len - 1, true)) {
    kmutexRUnlock(&currentThread->process->mutex);
    sfree(buf, len);
    return -1;
  }

  memcpy((void*)bufAddr, buf, actualLen);
  sfree(buf, len);
  kmutexRUnlock(&currentThread->process->mutex);

  return actualLen;
}
