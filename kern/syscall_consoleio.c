/** @file syscall_consoleio.c
 *
 *  @brief Syscalls about console I/O
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
  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseMultiParam(params, 0, &len)) {
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  if (len > MAX_READWRITE_BUFFER_SIZE || len <= 0) {
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  if (!parseMultiParam(params, 1, (int*)(&bufAddr))) {
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  if (!verifyUserSpaceAddr(bufAddr, bufAddr + len - 1, true)) {
    // The buffer itself is not writable or has problems
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

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

  kmutexWLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  // Revalidate, the page table may be removed when waiting for keyboard
  if (!verifyUserSpaceAddr(bufAddr, bufAddr + len - 1, true)) {
    kmutexWUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    sfree(buf, len);
    return -1;
  }

  memcpy((void*)bufAddr, buf, actualLen);
  sfree(buf, len);
  kmutexWUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  return actualLen;
}

// TODO test it
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
  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseMultiParam(params, 0, &len)) {
    // invalid len
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  if (len > MAX_READWRITE_BUFFER_SIZE) {
    // too long len
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  if (!parseMultiParam(params, 1, (int*)(&bufAddr))) {
    // invalid bufaddr
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }

  char* source = smalloc(len);
  if (!source) {
    // kernel runs out of memory
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }

  int actualLen = sGetString(bufAddr, source, len);
  if (actualLen == -1) {
    // invalid string
    sfree(source, len);
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  putbytes(source, actualLen);
  sfree(source, len);
  return 0;
}

// TODO test it
int set_term_color_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  int color;
  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseSingleParam(params, &color)) {
    // invalid color
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  return set_term_color(color);
}

// TODO test it
int set_cursor_pos_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  int row, col;
  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseMultiParam(params, 0, &row)) {
    // invalid row
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  if (!parseMultiParam(params, 1, &col)) {
    // invalid col
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  return set_cursor(row, col);
}

// TODO test it
int get_cursor_pos_Internal(SyscallParams params) {
  int row, col;
  get_cursor(&row, &col);

  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  int rowAddr, colAddr;
  kmutexWLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  if (!parseMultiParam(params, 0, &rowAddr)) {
    // invalid row address
    kmutexWUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  if (!parseMultiParam(params, 1, &colAddr)) {
    // invalid col address
    kmutexWUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  if (!verifyUserSpaceAddr(rowAddr, rowAddr + sizeof(int) - 1, true) ||
      !verifyUserSpaceAddr(colAddr, colAddr + sizeof(int) - 1, true)) {
    // row/col destination is not writable
    kmutexWUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  *((int*)rowAddr) = row;
  *((int*)colAddr) = col;
  kmutexWUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  return 0;
}
