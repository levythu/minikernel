#include <x86/seg.h>

.globl switchToRing3

# switchToRing3(esp, eflags, eip)
switchToRing3:
    mov %esp, %ecx

    mov $SEGSEL_USER_DS, %eax
    mov %ax, %ds
    mov %ax, %es
    pushl $SEGSEL_USER_DS  # ss
    pushl 4(%ecx) # esp
    pushl 8(%ecx) # eflags
    pushl $SEGSEL_USER_CS  # cs
    pushl 12(%ecx) # eip

    iret
