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

#define MAX_READWRITE_BUFFER_SIZE (CONSOLE_WIDTH * CONSOLE_HEIGHT)

int readline_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);

  int len;
  uint32_t bufAddr;
  kmutexRLock(&currentThread->process->mutex);
  if (!parseMultiParam(params, 0, &len)) {
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  if (len > MAX_READWRITE_BUFFER_SIZE) {
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

int getchar_Internal(SyscallParams params) {
  occupyKeyboard();
  int actualLen = getcharBlocking();
  releaseKeyboard();

  return actualLen;
}

int print_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);

  int len;
  uint32_t bufAddr;
  kmutexRLock(&currentThread->process->mutex);
  if (!parseMultiParam(params, 0, &len)) {
    // invalid len
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  if (len > MAX_READWRITE_BUFFER_SIZE) {
    // too long len
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  if (!parseMultiParam(params, 1, (int*)(&bufAddr))) {
    // invalid bufaddr
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }

  char* source = smalloc(len);
  if (!source) {
    // kernel runs out of memory
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }

  int actualLen = sGetString(bufAddr, source, len);
  if (actualLen == -1) {
    // invalid string
    sfree(source, len);
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  kmutexRUnlock(&currentThread->process->mutex);

  putbytes(source, actualLen);
  sfree(source, len);
  return 0;
}

int set_term_color_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  int color;
  kmutexRLock(&currentThread->process->mutex);
  if (!parseSingleParam(params, &color)) {
    // invalid color
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  kmutexRUnlock(&currentThread->process->mutex);

  return set_term_color(color);
}

int set_cursor_pos_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  int row, col;
  kmutexRLock(&currentThread->process->mutex);
  if (!parseMultiParam(params, 0, &row)) {
    // invalid row
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  if (!parseMultiParam(params, 0, &col)) {
    // invalid col
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  kmutexRUnlock(&currentThread->process->mutex);

  return set_cursor(row, col);
}

int get_cursor_pos_Internal(SyscallParams params) {
  int row, col;
  get_cursor(&row, &col);

  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  int rowAddr, colAddr;
  kmutexRLock(&currentThread->process->mutex);
  if (!parseMultiParam(params, 0, &rowAddr)) {
    // invalid row address
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  if (!parseMultiParam(params, 0, &colAddr)) {
    // invalid col address
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  if (!verifyUserSpaceAddr(rowAddr, rowAddr, true) ||
      !verifyUserSpaceAddr(colAddr, colAddr, true)) {
    // row/col destination is not writable
    kmutexRUnlock(&currentThread->process->mutex);
    return -1;
  }
  *((int*)rowAddr) = row;
  *((int*)colAddr) = col;
  kmutexRUnlock(&currentThread->process->mutex);

  return 0;
}
