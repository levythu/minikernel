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

static CrossCPULock latch;

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

  lprintf("Claim all user space frames, %d available", userPhysicalFrames);
}

// zero for out-of-memory
uint32_t getUserMemPage() {
  GlobalLockR(&latch);
  if (stackSize == 0) {
    GlobalUnlockR(&latch);
    return 0;
  }
  GlobalUnlockR(&latch);
  return availableFrameStack[--stackSize];
}

void freeUserMemPage(uint32_t mem) {
  assert(IS_PAGE_ALIGNED(mem));
  GlobalLockR(&latch);
  availableFrameStack[stackSize++] = mem;
  GlobalUnlockR(&latch);
}
