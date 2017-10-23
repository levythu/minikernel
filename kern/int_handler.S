#include <x86/seg.h>

.globl timerIntHandler
.globl keyboardIntHandler

timerIntHandler:
    pusha
    push %ds
    push %es
    mov $SEGSEL_KERNEL_DS, %eax
    mov %ax, %ds
    mov %ax, %es
    mov $0, %ebp
    call timerIntHandlerInternal
    pop %es
    pop %ds
    popa
    iret

keyboardIntHandler:
    pusha
    push %ds
    push %es
    mov $SEGSEL_KERNEL_DS, %eax
    mov %ax, %ds
    mov %ax, %es
    mov $0, %ebp
    call keyboardIntHandlerInternal
    pop %es
    pop %ds
    popa
    iret
