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

#define MAKE_SYSCALL_IDT(syscallName, syscallIntNumber) \
  do { \
    int32_t* idtBase = (int32_t*)idt_base(); \
    idtBase[syscallIntNumber  << 1] = ENCRYPT_IDT_TRAPGATE_LSB( \
      3, (int32_t)(syscallName ## _Handler), 1, SEGSEL_KERNEL_CS, 1); \
    idtBase[(syscallIntNumber  << 1) + 1] = ENCRYPT_IDT_TRAPGATE_MSB( \
      3, (int32_t)(syscallName ## _Handler), 1, SEGSEL_KERNEL_CS, 1); \
  } while (false)

void initSyscall() {
  MAKE_SYSCALL_IDT(gettid, GETTID_INT);

  lprintf("Registered all system call handler.");
}

int gettid_Internal() {
  lprintf("GETTID");
  return 0;
}