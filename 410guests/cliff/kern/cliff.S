/** @file cliff.S
 *  @brief Exception delivery will try to push information onto the stack.
 *  If pushing fails, then the guest should die.  We install a (fake) IDT
 *  entry for the divide-by-zero exception, then we divide by zero.
 * 
 *  @author rpearl
 */

#include <x86/idt.h>
#include <hvcall_int.h>

.globl kernel_main
kernel_main:
    push $0                  # Not Privileged
	push $0x15410            # not a valid handler, but we don't care
	push $IDT_DE
	mov $HV_SETIDT_OP, %eax
	int $HV_INT              # ok, "handler" is installed

	mov $0xFFF00000, %esp        # bad stack pointer

	xor %ecx, %ecx
	idiv %ecx                # game over
