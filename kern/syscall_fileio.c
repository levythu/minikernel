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
#include "page.h"

#define FILE_IO_BUFFER PAGE_SIZE
#define MAX_ACCEPTABLE_FILENAME_LEN 256

int readfile_Internal(SyscallParams params) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);

  int len, offset;
  uint32_t filename, buf;
  kmutexRLock(&currentThread->process->memlock);

  // Validate & get filename
  if (!parseMultiParam(params, 0, (int*)(&filename))) {
    kmutexRUnlock(&currentThread->process->memlock);
    // invalid filename address
    return -1;
  }
  char* filenameKernel = smalloc(MAX_ACCEPTABLE_FILENAME_LEN);
  if (!filenameKernel) {
    kmutexRUnlock(&currentThread->process->memlock);
    // no free kernel space
    return -1;
  }
  int actualLengthForFilename =
      sGetString(filename, filenameKernel, MAX_ACCEPTABLE_FILENAME_LEN);
  if (actualLengthForFilename < 0 ||
      actualLengthForFilename == MAX_ACCEPTABLE_FILENAME_LEN) {
    // filename not valid or too long
    kmutexRUnlock(&currentThread->process->memlock);
    sfree(filenameKernel, MAX_ACCEPTABLE_FILENAME_LEN);
  }

  // validate buffer, but the write permission check is delayed to write time
  if (!parseMultiParam(params, 1, (int*)(&buf))) {
    kmutexRUnlock(&currentThread->process->memlock);
    sfree(filenameKernel, MAX_ACCEPTABLE_FILENAME_LEN);
    // invalid buffer address
    return -1;
  }

  // validate len
  if (!parseMultiParam(params, 2, &len)) {
    kmutexRUnlock(&currentThread->process->memlock);
    sfree(filenameKernel, MAX_ACCEPTABLE_FILENAME_LEN);
    // invalid len
    return -1;
  }
  if (len < 0) {
    kmutexRUnlock(&currentThread->process->memlock);
    sfree(filenameKernel, MAX_ACCEPTABLE_FILENAME_LEN);
    // invalid len
    return -1;
  }

  // validate offset
  if (!parseMultiParam(params, 3, &offset)) {
    kmutexRUnlock(&currentThread->process->memlock);
    sfree(filenameKernel, MAX_ACCEPTABLE_FILENAME_LEN);
    // invalid offset
    return -1;
  }
  if (offset < 0) {
    kmutexRUnlock(&currentThread->process->memlock);
    sfree(filenameKernel, MAX_ACCEPTABLE_FILENAME_LEN);
    // invalid offset
    return -1;
  }
  kmutexRUnlock(&currentThread->process->memlock);

  // start reading files chunk by chunk
  int byteRead = 0;
  char* bufferKernel = smalloc(FILE_IO_BUFFER);
  while (len > 0) {
    int byteToRead = FILE_IO_BUFFER < len ? FILE_IO_BUFFER : len;
    int actualRead = getbytes(filenameKernel, offset, byteToRead, bufferKernel);
    if (actualRead < 0) {
      // filename wrong, or go beyond the file, etc.
      if (byteRead > 0) {
        // we have read something, it's due to this chunk is going out. Just
        // return
        break;
      } else {
        // filename's wrong, or offset is too large. go error
        sfree(filenameKernel, MAX_ACCEPTABLE_FILENAME_LEN);
        sfree(bufferKernel, FILE_IO_BUFFER);
        return -1;
      }
    } else if (actualRead == 0) {
      // nothing more can be read.
      break;
    } else {
      // lock the memlock, copy data
      kmutexRLock(&currentThread->process->memlock);
      // use byteToRead as check length instead of byteRead. Since this is what
      // user thinks it should be valid
      if (!verifyUserSpaceAddr(buf + offset,
                               buf + offset + byteToRead - 1, true)) {
        // user space is not available to write
        kmutexRUnlock(&currentThread->process->memlock);
        sfree(filenameKernel, MAX_ACCEPTABLE_FILENAME_LEN);
        sfree(bufferKernel, FILE_IO_BUFFER);
        return -1;
      }
      memcpy((void*)(buf + offset), bufferKernel, actualRead);
      kmutexRUnlock(&currentThread->process->memlock);

      byteRead += actualRead;
      offset += actualRead;
      len -= actualRead;
    }
  }

  sfree(filenameKernel, MAX_ACCEPTABLE_FILENAME_LEN);
  sfree(bufferKernel, FILE_IO_BUFFER);
  return byteRead;
}
