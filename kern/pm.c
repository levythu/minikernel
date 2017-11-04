/** @file pm.c
 *
 *  @brief Physical Memory manager, only for non-kernel space.
 *
 *  All free pages are kept in a stack, and latch serves as a global lock to
 *  protect the data structure
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>

#include "x86/asm.h"
#include "x86/cr.h"
#include "common_kern.h"
#include "pm.h"
#include "vm.h"
#include "bool.h"
#include "cpu.h"

static int physicalFrames;
static int userPhysicalFrames;

static uint32_t* availableFrameStack;
static int stackSize;
static int reservedSize;

static CrossCPULock latch;

static uint32_t ZFODBlock;

bool isZFOD(uint32_t addr) {
  return addr == ZFODBlock;
}

void claimUserMem() {
  initCrossCPULock(&latch);
  physicalFrames = machine_phys_frames();
  userPhysicalFrames = physicalFrames - USER_MEM_START / PAGE_SIZE;
  availableFrameStack = smalloc(sizeof(uint32_t) * userPhysicalFrames);

  uint32_t basicUserAddr = USER_MEM_START;
  for (int i=0; i<userPhysicalFrames; i++) {
    availableFrameStack[i] = basicUserAddr;
    basicUserAddr += PAGE_SIZE;
  }
  stackSize = userPhysicalFrames;
  reservedSize = 0;

  ZFODBlock = getUserMemPage();
  assert(ZFODBlock != 0);
  memset((void*)ZFODBlock, 0, PAGE_SIZE);

  lprintf("Claim all user space frames, %d available", userPhysicalFrames);
}

uint32_t getUserMemPageZFOD() {
  GlobalLockR(&latch);
  if (stackSize == reservedSize) {
    GlobalUnlockR(&latch);
    return 0;
  }
  reservedSize++;
  GlobalUnlockR(&latch);
  return ZFODBlock;
}

uint32_t upgradeUserMemPageZFOD(uint32_t mem) {
  // There must be at lease one reserved for me!
  GlobalLockR(&latch);
  assert(reservedSize > 0);
  uint32_t res = availableFrameStack[--reservedSize];
  // Put swap it with the stack top, so that the stack top can be valid, and
  // upgraded block gets outside stack

  availableFrameStack[reservedSize] = availableFrameStack[--stackSize];
  availableFrameStack[stackSize] = res;

  GlobalUnlockR(&latch);
  return res;
}

// zero for out-of-memory
uint32_t getUserMemPage() {
  GlobalLockR(&latch);
  if (stackSize == reservedSize) {
    GlobalUnlockR(&latch);
    return 0;
  }
  uint32_t res = availableFrameStack[--stackSize];
  GlobalUnlockR(&latch);
  return res;
}

void freeUserMemPage(uint32_t mem) {
  assert(IS_PAGE_ALIGNED(mem));
  GlobalLockR(&latch);
  if (ZFODBlock == mem) {
    // return a ZFOD blocked, just de-reserve
    assert(reservedSize > 0);
    reservedSize--;
  } else {
    availableFrameStack[stackSize++] = mem;
  }
  GlobalUnlockR(&latch);
}
