/*
 * TODO
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

DECLARE_SYSCALL_WRAPPER(new_pages);
DECLARE_SYSCALL_WRAPPER(remove_pages);

void initSyscall();

// Parsing process is not atomic: some memory may be freed by other thread, so
// memlock should be acquired, unless you know that there's only one thread
bool parseSingleParam(SyscallParams params, int* result);
bool parseMultiParam(SyscallParams params, int argnum, int* result);

#endif
