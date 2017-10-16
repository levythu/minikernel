/*
 * TODO
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "make_syscall_handler.h"

DECLARE_SYSCALL_WRAPPER(gettid);

void initSyscall();

#endif
