/*
 * TODO
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "make_syscall_handler.h"

// To add a syscall handler, you need:
// 1. DECLARE_SYSCALL_WRAPPER(syscall-name) in syscall.h
// 2. MAKE_SYSCALL_WRAPPER(syscall-name) in syscall_handler.S
// 3. MAKE_SYSCALL_IDT(syscall-name, syscall-int-number) in
//      syscall.c::initSyscall()
// 4. Write your implementation of syscall-name_Internal somewhere

DECLARE_SYSCALL_WRAPPER(gettid);
DECLARE_SYSCALL_WRAPPER(fork);
DECLARE_SYSCALL_WRAPPER(set_status);
DECLARE_SYSCALL_WRAPPER(wait);
DECLARE_SYSCALL_WRAPPER(vanish);

void initSyscall();

#endif
