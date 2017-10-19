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

#include "x86/asm.h"
#include "x86/cr.h"
#include "x86/seg.h"
#include "common_kern.h"
#include "syscall.h"
#include "int_handler.h"
#include "bool.h"
#include "cpu.h"

int gettid_Internal() {
  return getLocalCPU()->runningTID;
}

int fork_Internal() {
  return -1;
}
