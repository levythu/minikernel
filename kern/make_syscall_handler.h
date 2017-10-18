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

// TODO ds and es
#define MAKE_SYSCALL_WRAPPER(syscallName)                          \
    .globl syscallName ## _Handler;                         \
    syscallName ## _Handler:                                \
      pusha;                                                       \
      call syscallName ## _Internal;                        \
      movl %eax, (%esp);                                            \
      popa;                                                        \
      iret;


#define DECLARE_SYSCALL_WRAPPER(syscallName)    \
    void syscallName ## _Handler ();             \
    int syscallName ## _Internal ();

#endif
