/** @file cpu.c
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
#include "cpu.h"
#include "bool.h"

static cpu UniCore;

void initCPU() {
  UniCore.id = 0;
  UniCore.interruptSwitch = false;
  UniCore.runningTID = -1;
  UniCore.runningPID = -1;
  UniCore.currentMutexLayer = -1;
  lprintf("CPU env initiated, count = 1");
}

cpu* getLocalCPU() {
  return &UniCore;
}

//==============================================================================
// General purpose functions

void DisableInterrupts() {
  disable_interrupts();
  getLocalCPU()->interruptSwitch = false;
}

void EnableInterrupts() {
  getLocalCPU()->interruptSwitch = true;
  enable_interrupts();
}

void LocalLockR() {
  DisableInterrupts();
  getLocalCPU()->currentMutexLayer++;
}

void LocalUnlockR() {
  assert(!getLocalCPU()->interruptSwitch);
  getLocalCPU()->currentMutexLayer--;
  if (getLocalCPU()->currentMutexLayer < 0) {
    EnableInterrupts();
  }
}

void initCrossCPULock(CrossCPULock* lock) {
  lock->holderCPU = -1;
  lock->currentMutexLayer = -1;
}

void GlobalLockR(CrossCPULock* lock) {
  LocalLockR();
  int id = getLocalCPU()->id;
  if (lock->holderCPU == id) {
    // We are already in, nothing has to be done.
    // Also, we are not afraid being interleaved, since local lock is on
    lock->currentMutexLayer++;
  } else {
    while (!__sync_bool_compare_and_swap(&lock->holderCPU, -1, id))
      ;
    assert(lock->currentMutexLayer == -1);
    lock->currentMutexLayer = 0;
  }
}

void GlobalUnlockR(CrossCPULock* lock) {
  int id = getLocalCPU()->id;
  assert(lock->holderCPU == id);
  lock->currentMutexLayer--;
  if (lock->currentMutexLayer < 0) {
    lock->holderCPU = -1;
  }
  LocalUnlockR();
}
