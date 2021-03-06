/** @file syscall_memory.c
 *
 *  @brief Syscalls memory management
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
#include "vm.h"
#include "pm.h"
#include "source_untrusted.h"

#define PAGE_STAMP_USR_HEAD 1
#define PAGE_STAMP_USR_BODY 2

// Not multithread safe, must protected under process-memlock
// Register new user page and stamp then in the page table, start from base with
// length len. base and len must page aligned
// Maybe partial success, return value = endAddr, so that [base, endAddr) is
// registered, and the caller may want to rollback
static uint32_t _registerNewPage(PageDirectory pd, uint32_t base,
    uint32_t len) {
  for (uint32_t currentPage = base; currentPage != base + len;
      currentPage += PAGE_SIZE) {
    // overlap is okay!
    if (searchPTEntryPageDirectory(pd, currentPage)) {
      // current address is presented, abort!
      return currentPage;
    }
    uint32_t pm = getUserMemPageZFOD();
    if (!pm) {
      // Huh, no free memory available, abort!
      return currentPage;
    }
    // We are good to go
    createMapPageDirectory(pd, currentPage, pm, true, false);
    PTE* createdPTE = searchPTEntryPageDirectory(pd, currentPage);
    // stamp the page table (use bit 9/10/11)
    *createdPTE |= PE_ENCODE_CUSTOM(currentPage == base ?
        PAGE_STAMP_USR_HEAD : PAGE_STAMP_USR_BODY);
  }
  return base + len;
}

// We judge the length by page stamp!
static bool _unregisterNewPage(PageDirectory pd, uint32_t base) {
  for (uint32_t currentPage = base; /* NO END! */; currentPage += PAGE_SIZE) {
    PTE* createdPTE = searchPTEntryPageDirectory(pd, currentPage);
    if (currentPage == base &&
        PE_DECODE_CUSTOM(*createdPTE) != PAGE_STAMP_USR_HEAD) {
      // You liar, it's not the head of user allocated memory
      return false;
    }
    if (currentPage != base &&
        PE_DECODE_CUSTOM(*createdPTE) != PAGE_STAMP_USR_BODY) {
      // Oh, we are done!
      return true;
    }
    freeUserMemPage(PE_DECODE_ADDR(*createdPTE));
    *createdPTE = PE_PRESENT(0) | PE_WRITABLE(0) | PE_USERMODE(0) |
                  PE_WRITETHROUGH_CACHE(0) | PE_DISABLE_CACHE(0) |
                  PE_SIZE_FLAG(0);
    invalidateTLB(currentPage);
  }
  // No we ain't gonna reach here
  return false;
}

int new_pages_Internal(SyscallParams params) {
  // We own currentThread
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);

  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  uint32_t base, len;
  if (!parseMultiParam(params, 0, (int*)&base) ||
      !parseMultiParam(params, 1, (int*)&len)) {
    // Invalid param
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  if (len == 0 || !IS_PAGE_ALIGNED(base) || !IS_PAGE_ALIGNED(len)) {
    // invalid length or base
    return -1;
  }

  kmutexWLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  uint32_t end = _registerNewPage(currentThread->process->pd, base, len);
  if (end != base + len) {
    // partial success, we roll back
    if (end != base) _unregisterNewPage(currentThread->process->pd, base);
    kmutexWUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  // We are done!
  kmutexWUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  return 0;
}

// We take advantage of custom bits in page table entry to mark allocated memory
// and boundary. PAGE_STAMP_USR_HEAD for the first page allocated in one call,
// and PAGE_STAMP_USR_BODY for the rest pages allocated in one call.
int remove_pages_Internal(SyscallParams params) {
  // We own currentThread
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);

  kmutexRLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  uint32_t base;
  if (!parseSingleParam(params, (int*)&base)) {
    // Invalid param
    kmutexRUnlockRecord(&currentThread->process->memlock,
        &currentThread->memLockStatus);
    return -1;
  }
  kmutexRUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  if (!IS_PAGE_ALIGNED(base)) {
    // invalid length or base
    return -1;
  }

  kmutexWLockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);
  bool result = _unregisterNewPage(currentThread->process->pd, base);
  kmutexWUnlockRecord(&currentThread->process->memlock,
      &currentThread->memLockStatus);

  return result ? 0 : -1;
}
