/** @file mode_switch.S
 *
 *  @brief Infrastructure for mode switch
 *
 *  Implement mode switch with iret
 *
 *  @author Leiyu Zhao
 */

#include <x86/seg.h>

.globl switchToRing3
.globl switchToRing3X

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


# switchToRing3X(esp, eflags, eip, edi, esi, ebp, ebx, edx, ecx, eax, cs, ds)
switchToRing3X:
    mov %esp, %ecx

    mov 48(%ecx), %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    pushl %eax  # ss
    pushl 4(%ecx) # esp
    pushl 8(%ecx) # eflags
    mov 44(%ecx), %eax
    pushl %eax  # cs
    pushl 12(%ecx) # eip

    mov 16(%ecx), %edi
    mov 20(%ecx), %esi
    mov 24(%ecx), %ebp
    mov 28(%ecx), %ebx
    mov 32(%ecx), %edx
    mov 40(%ecx), %eax
    mov 36(%ecx), %ecx

    iret
