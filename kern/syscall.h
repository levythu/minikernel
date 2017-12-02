/** @file syscall.h
 *
 *  @brief Syscall module
 *
 *  @author Leiyu Zhao
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#include "make_syscall_handler.h"
#include "bool.h"

// To add a syscall handler, you need:
// 1. DECLARE_SYSCALL_WRAPPER(syscall-name) in syscall.h
// 2. MAKE_SYSCALL_WRAPPER(syscall-name) in syscall_handler.S
// 3. MAKE_SYSCALL_IDT(syscall-name, syscall-int-number) in
//      syscall.c::initSyscall()
// 4. Write your implementation of syscall-name_Internal somewhere

typedef uint32_t SyscallParams;

DECLARE_SYSCALL_WRAPPER(gettid);
DECLARE_SYSCALL_WRAPPER(fork);
DECLARE_SYSCALL_WRAPPER(set_status);
DECLARE_SYSCALL_WRAPPER(wait);
DECLARE_SYSCALL_WRAPPER(vanish);
DECLARE_SYSCALL_WRAPPER(exec);
DECLARE_SYSCALL_WRAPPER(task_vanish);

DECLARE_SYSCALL_WRAPPER(new_pages);
DECLARE_SYSCALL_WRAPPER(remove_pages);

DECLARE_SYSCALL_WRAPPER(readline);
DECLARE_SYSCALL_WRAPPER(getchar);
DECLARE_SYSCALL_WRAPPER(print);
DECLARE_SYSCALL_WRAPPER(set_term_color);
DECLARE_SYSCALL_WRAPPER(set_cursor_pos);
DECLARE_SYSCALL_WRAPPER(get_cursor_pos);
DECLARE_SYSCALL_WRAPPER(new_console);

DECLARE_SYSCALL_WRAPPER(swexn);
DECLARE_SYSCALL_WRAPPER(sleep);
DECLARE_SYSCALL_WRAPPER(yield);
DECLARE_SYSCALL_WRAPPER(make_runnable);
DECLARE_SYSCALL_WRAPPER(deschedule);
DECLARE_SYSCALL_WRAPPER(thread_fork);
DECLARE_SYSCALL_WRAPPER(get_ticks);

DECLARE_SYSCALL_WRAPPER(readfile);
DECLARE_SYSCALL_WRAPPER(halt);
DECLARE_SYSCALL_WRAPPER(misbehave);

// Install syscall handler. Must be called in kernel setup
void initSyscall();

// Parsing process is not atomic: some memory may be freed by other thread, so
// memlock should be acquired, unless you know that there's only one thread
bool parseSingleParam(SyscallParams params, int* result);
bool parseMultiParam(SyscallParams params, int argnum, int* result);

// Following are dummy handler just for hypervisor

// 65~116, 128~134
void hvHd_65();
void hvHd_66();
void hvHd_67();
void hvHd_68();
void hvHd_69();
void hvHd_70();
void hvHd_71();
void hvHd_72();
void hvHd_73();
void hvHd_74();
void hvHd_75();
void hvHd_76();
void hvHd_77();
void hvHd_78();
void hvHd_79();
void hvHd_80();
void hvHd_81();
void hvHd_82();
void hvHd_83();
void hvHd_84();
void hvHd_85();
void hvHd_86();
void hvHd_87();
void hvHd_88();
void hvHd_89();
void hvHd_90();
void hvHd_91();
void hvHd_92();
void hvHd_93();
void hvHd_94();
void hvHd_95();
void hvHd_96();
void hvHd_97();
void hvHd_98();
void hvHd_99();
void hvHd_100();
void hvHd_101();
void hvHd_102();
void hvHd_103();
void hvHd_104();
void hvHd_105();
void hvHd_106();
void hvHd_107();
void hvHd_108();
void hvHd_109();
void hvHd_110();
void hvHd_111();
void hvHd_112();
void hvHd_113();
void hvHd_114();
void hvHd_115();
void hvHd_116();

void hvHd_128();
void hvHd_129();
void hvHd_130();
void hvHd_131();
void hvHd_132();
void hvHd_133();
void hvHd_134();


#endif
