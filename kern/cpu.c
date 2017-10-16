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
  UniCore.interruptSwitch = false;
  UniCore.runningTID = -1;
  UniCore.runningPID = -1;
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
