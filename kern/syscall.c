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
#include "source_untrusted.h"

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
  MAKE_SYSCALL_IDT(fork, FORK_INT);
  MAKE_SYSCALL_IDT(set_status, SET_STATUS_INT);
  MAKE_SYSCALL_IDT(wait, WAIT_INT);
  MAKE_SYSCALL_IDT(vanish, VANISH_INT);
  MAKE_SYSCALL_IDT(exec, EXEC_INT);

  MAKE_SYSCALL_IDT(new_pages, NEW_PAGES_INT);
  MAKE_SYSCALL_IDT(remove_pages, REMOVE_PAGES_INT);

  MAKE_SYSCALL_IDT(readline, READLINE_INT);
  MAKE_SYSCALL_IDT(getchar, GETCHAR_INT);
  MAKE_SYSCALL_IDT(print, PRINT_INT);
  MAKE_SYSCALL_IDT(set_term_color, SET_TERM_COLOR_INT);
  MAKE_SYSCALL_IDT(set_cursor_pos, SET_CURSOR_POS_INT);
  MAKE_SYSCALL_IDT(get_cursor_pos, GET_CURSOR_POS_INT);

  MAKE_SYSCALL_IDT(swexn, SWEXN_INT);
  MAKE_SYSCALL_IDT(sleep, SLEEP_INT);
  MAKE_SYSCALL_IDT(yield, YIELD_INT);
  MAKE_SYSCALL_IDT(make_runnable, MAKE_RUNNABLE_INT);
  MAKE_SYSCALL_IDT(deschedule, DESCHEDULE_INT);
  MAKE_SYSCALL_IDT(thread_fork, THREAD_FORK_INT);

  MAKE_SYSCALL_IDT(readfile, READFILE_INT);
  MAKE_SYSCALL_IDT(halt, HALT_INT);
  MAKE_SYSCALL_IDT(misbehave, MISBEHAVE_INT);

  lprintf("Registered all system call handler.");
}

/******************************************************************************/
// Helper functions for parsing params

bool parseSingleParam(SyscallParams params, int* result) {
  *result = (int)params;
  return true;
}

bool parseMultiParam(SyscallParams params, int argnum, int* result) {
  return sGetInt(params + 4 * argnum, result);
}
