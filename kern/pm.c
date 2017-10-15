/** @file pm.c
 *
 *  @brief TODO

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

static int physicalFrames;
static int userPhysicalFrames;

static uint32_t* availableFrameStack;
static int stackSize;

void claimUserMem() {
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
  if (stackSize == 0) return 0;
  return availableFrameStack[--stackSize];
}

void freeUserMemPage(uint32_t mem) {
  assert(IS_PAGE_ALIGNED(mem));
  availableFrameStack[stackSize++] = mem;
}
