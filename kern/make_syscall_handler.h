/** @file make_syscall_handler.h
 *
 *  @brief Macros to batch the syscall entrance.
 *
 *  For each syscallName, MAKE_SYSCALL_WRAPPER(syscall) will generate a
 *  syscall_Handler which is good to be provided to IDT, and it will delegate
 *  all internal logics to syscall_Interal
 *
 *  DECLARE_SYSCALL_WRAPPER will help declar them.
 *
 *  @author Leiyu Zhao
 */

#ifndef MAKE_SYSCALL_HANDLER_H
#define MAKE_SYSCALL_HANDLER_H

#include <x86/seg.h>

// On syscall_Interal returns, the stack is:
// EAX, ECX, EDX, EBX, EBP, ESP, EBP, ESI, EDI, DS, ES
//                                                  ^
//                                                 %esp
#define MAKE_SYSCALL_WRAPPER(syscallName)                          \
    .globl syscallName ## _Handler;                         \
    syscallName ## _Handler:                                \
      pusha;                                                       \
      push %ds;                        \
      push %es;                          \
      mov $SEGSEL_KERNEL_DS, %eax;     \
      mov %ax, %ds;                    \
      mov %ax, %es;                    \
      call syscallName ## _Internal;                        \
      pop %es;  \
      pop %ds;  \
      movl %eax, 28(%esp);                                            \
      popa;                                                        \
      iret;


#define DECLARE_SYSCALL_WRAPPER(syscallName)    \
    void syscallName ## _Handler ();             \
    int syscallName ## _Internal ();

#endif
