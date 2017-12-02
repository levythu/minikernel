/** @file syscall.c
 *
 *  @brief Syscall module, used for set up syscalls and helpers
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

#define MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(handlerName, syscallIntNumber) \
  do { \
    int32_t* idtBase = (int32_t*)idt_base(); \
    idtBase[syscallIntNumber  << 1] = ENCRYPT_IDT_TRAPGATE_LSB( \
      3, (int32_t)(handlerName), 1, SEGSEL_KERNEL_CS, 1); \
    idtBase[(syscallIntNumber  << 1) + 1] = ENCRYPT_IDT_TRAPGATE_MSB( \
      3, (int32_t)(handlerName), 1, SEGSEL_KERNEL_CS, 1); \
  } while (false)

static void reigsterHypervisorOnlyHandler();
void initSyscall() {
  // Register hypervisor only handler eariler, so that it can be overridden
  // by normal syscall handler below
  reigsterHypervisorOnlyHandler();

  MAKE_SYSCALL_IDT(gettid, GETTID_INT);
  MAKE_SYSCALL_IDT(fork, FORK_INT);
  MAKE_SYSCALL_IDT(set_status, SET_STATUS_INT);
  MAKE_SYSCALL_IDT(wait, WAIT_INT);
  MAKE_SYSCALL_IDT(vanish, VANISH_INT);
  MAKE_SYSCALL_IDT(exec, EXEC_INT);
  MAKE_SYSCALL_IDT(task_vanish, TASK_VANISH_INT);

  MAKE_SYSCALL_IDT(new_pages, NEW_PAGES_INT);
  MAKE_SYSCALL_IDT(remove_pages, REMOVE_PAGES_INT);

  MAKE_SYSCALL_IDT(readline, READLINE_INT);
  MAKE_SYSCALL_IDT(getchar, GETCHAR_INT);
  MAKE_SYSCALL_IDT(print, PRINT_INT);
  MAKE_SYSCALL_IDT(set_term_color, SET_TERM_COLOR_INT);
  MAKE_SYSCALL_IDT(set_cursor_pos, SET_CURSOR_POS_INT);
  MAKE_SYSCALL_IDT(get_cursor_pos, GET_CURSOR_POS_INT);
  MAKE_SYSCALL_IDT(new_console, NEW_CONSOLE_INT);

  MAKE_SYSCALL_IDT(swexn, SWEXN_INT);
  MAKE_SYSCALL_IDT(sleep, SLEEP_INT);
  MAKE_SYSCALL_IDT(yield, YIELD_INT);
  MAKE_SYSCALL_IDT(make_runnable, MAKE_RUNNABLE_INT);
  MAKE_SYSCALL_IDT(deschedule, DESCHEDULE_INT);
  MAKE_SYSCALL_IDT(thread_fork, THREAD_FORK_INT);
  MAKE_SYSCALL_IDT(get_ticks, GET_TICKS_INT);

  MAKE_SYSCALL_IDT(readfile, READFILE_INT);
  MAKE_SYSCALL_IDT(halt, HALT_INT);
  MAKE_SYSCALL_IDT(misbehave, MISBEHAVE_INT);

  lprintf("Registered all system call handler.");
}

static void reigsterHypervisorOnlyHandler() {
  // 65~116, 128~134
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_65, 65);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_66, 66);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_67, 67);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_68, 68);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_69, 69);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_70, 70);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_71, 71);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_72, 72);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_73, 73);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_74, 74);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_75, 75);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_76, 76);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_77, 77);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_78, 78);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_79, 79);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_80, 80);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_81, 81);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_82, 82);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_83, 83);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_84, 84);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_85, 85);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_86, 86);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_87, 87);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_88, 88);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_89, 89);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_90, 90);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_91, 91);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_92, 92);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_93, 93);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_94, 94);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_95, 95);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_96, 96);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_97, 97);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_98, 98);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_99, 99);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_100, 100);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_101, 101);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_102, 102);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_103, 103);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_104, 104);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_105, 105);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_106, 106);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_107, 107);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_108, 108);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_109, 109);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_110, 110);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_111, 111);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_112, 112);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_113, 113);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_114, 114);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_115, 115);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_116, 116);

  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_128, 128);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_129, 129);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_130, 130);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_131, 131);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_132, 132);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_133, 133);
  MAKE_SYSCALL_IDT_HYPERVISOR_ONLY(hvHd_134, 134);
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
