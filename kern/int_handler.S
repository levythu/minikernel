#include <x86/seg.h>

.globl timerIntHandler
.globl keyboardIntHandler

timerIntHandler:
    pusha
    pushl %ds
    pushl %es
    mov $SEGSEL_KERNEL_DS, %eax
    mov %ax, %ds
    mov %ax, %es
    mov $0, %ebp
    call timerIntHandlerInternal
    call hypervisorTimerHook
    popl %es
    popl %ds
    popa
    iret

keyboardIntHandler:
    pusha
    pushl %ds
    pushl %es
    mov $SEGSEL_KERNEL_DS, %eax
    mov %ax, %ds
    mov %ax, %es
    mov $0, %ebp
    call keyboardIntHandlerInternal
    popl %es
    popl %ds
    popa
    iret
