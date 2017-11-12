/** @file source_untrusted.c
 *
 *  @brief Helper utilities for validate and get data from untrusted source
 *
 *  All function inside is non-thread safe nor interrupt safe. Generally the
 *  caller use memlock to protect thread safeness, while avoid calling it inside
 *  interrupt handler to allow interrupt unsafeness.
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <stdint.h>

#include "common_kern.h"
#include "pm.h"
#include "vm.h"
#include "bool.h"
#include "process.h"

static bool verifyUserSpaceAddrGivenPD(uint32_t startAddr, uint32_t endAddr,
    bool mustWritable, PageDirectory mypd) {
  if (startAddr < USER_MEM_START) {
    // Part of the space is in kernel, invalid
    return false;
  }
  uint32_t lastVerifiedPageNum = 0;
  for (uint32_t i = startAddr; ; i++) {
    uint32_t myPageNum = PE_DECODE_ADDR(i);
    if (myPageNum != lastVerifiedPageNum) {
      lastVerifiedPageNum = myPageNum;

      if (myPageNum < USER_MEM_START) {
        return false;
      }

      PTE* targetPTE = searchPTEntryPageDirectory(mypd, lastVerifiedPageNum);
      if (!targetPTE) {
        // The page does not exist at all
        return false;
      }

      if (mustWritable && !PE_IS_WRITABLE(*targetPTE) && !isZFOD(PE_DECODE_ADDR(*targetPTE))) {
        // We want a writable page, but it's not writable for user
        return false;
      }
    }

    if (i == endAddr) {
      // We cannot use for loop to detect overrange. endAddr may be 0xffffffff!
      break;
    }
  }
  return true;
}

bool verifyUserSpaceAddr(
    uint32_t startAddr, uint32_t endAddr, bool mustWritable) {
  PageDirectory mypd = findPCB(getLocalCPU()->runningPID)->pd;
  return verifyUserSpaceAddrGivenPD(startAddr, endAddr, mustWritable, mypd);
}

// It is not atomic, another thread may use syscall to change the memory.
// But since the memory can only grow, so it's safe
bool sGetInt(uint32_t addr, int* target) {
  if (!verifyUserSpaceAddr(addr, addr - 1 + sizeof(int), false)) return false;
  if (target) *target = *((int*)addr);
  return true;
}

// TODO: bug in :88!
#define sGetTypeArray(FuncName, TYPE) \
  int FuncName(uint32_t addr, TYPE* target, int size) { \
    PageDirectory mypd = findPCB(getLocalCPU()->runningPID)->pd; \
    int lastVerifiedPageNum = 0; \
    for (int i = 0; i < size * sizeof(TYPE); i++) { \
      if (lastVerifiedPageNum != PE_DECODE_ADDR(addr + i) && \
          !verifyUserSpaceAddrGivenPD(addr + i, addr + i, false, mypd)) { \
        return -1; \
      } \
      lastVerifiedPageNum = PE_DECODE_ADDR(addr + i); \
 \
      if (i % sizeof(TYPE) == 0) { \
        TYPE tmp = *((TYPE*)(addr + i)); \
        if (target) target[i] = tmp; \
        if (tmp == 0) { \
          return i / sizeof(TYPE); \
        } \
      } \
    } \
    return size; \
  }

sGetTypeArray(sGetString, char);

sGetTypeArray(sGeUIntArray, uint32_t);
