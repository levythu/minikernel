/** @file warp.S
 *
 *  @brief Just point our stack at the top of the indirectly mapped frame,
 *  trigger a hypercall, and clean up after ourselves.
 *
 *  @author rpearl
 */
#include <hvcall_int.h>
.globl warp

warp_ascii:
.ascii "Hello down there!"

warp:
	push %ebp
	mov %esp, %ebp
	mov $0x01000fff, %esp
	push $warp_ascii
	push $17
	mov $HV_PRINT_OP, %eax
	int $HV_INT
	mov %ebp, %esp
	pop %ebp
	ret

