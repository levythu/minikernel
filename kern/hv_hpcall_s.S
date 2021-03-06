#include <x86/seg.h>

.globl hyperCallHandler

hyperCallHandler:
    push %ecx
    mov 16(%esp), %ecx        # user %esp
    pusha
    pushl %ds
    pushl %es

    mov $SEGSEL_KERNEL_DS, %ebx
    mov %bx, %ds
    mov %bx, %es
    mov $0, %ebp

    push %eax
    push %ecx
    call hyperCallHandler_Internal
    pop %ecx
    pop %ecx            # we want to preserve %eax here

    popl %es
    popl %ds
    movl %eax, 28(%esp)
    popa
    pop %ecx
    iret
