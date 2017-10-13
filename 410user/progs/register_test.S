/** @file 410user/progs/register_test.S
 *  @author mpa
 *  @author ctokar
 *  @brief Tests that registers are saved over context switches.
 *  @public yes
 *  @for p3
 *  @covers ctxsw
 *  @status needswork
 */

#include "410_tests.h"
	
.global main
	.section	.rodata

test_name:	
	.string "register_test:"
str:	
	.string	"%s%s%s"
test_start:	
	.string TEST_START_4EVER
test_pfx:
	.string TEST_PFX
test_end_fail:	
	.string TEST_END_FAIL

.align 4	
eax:

	.long	0x17
ebx:	
	.long	0x42
ecx:
	.long	0xffffffff
edx:
	.long	0xc001d00d
esi:
	.long	0x9999abcd
edi:
	.long	0x12345678
ebp:
	.long	0x87654321
esp:
	.long	0xbaaaaaaa
eax2:
  .long 0x20
ebx2:
  .long 0x45
ecx2:
  .long 0x01010101
edx2:
  .long 0xbeef0011
esi2:
  .long 0xfeedcafe
edi2:
  .long 0x98765432
ebp2:
  .long 0x12457802
esp2:
  .long 0xabcdefab
str2:
	.string "%s%sOoops:	%%eax=0x%x, %%ebx=0x%x, %%ecx=0x%x, %%edx=0x%x, %%esi=0x%x, %%edi=0x%x, %%esp=0x%x, %%ebp=0x%x\n"
	.text
.comm	oldesp,4,4	
.comm	oldebp,4,4
.comm	newesp,4,4
.comm	newebp,4,4

main:
	movl	%esp,oldesp
	movl	%ebp,oldebp

	movl	%esp,%ebp			
	pushl	$test_start
	pushl	$test_name
	pushl	$test_pfx
	pushl	$str
	call	lprintf

  call  fork
  cmp   $0, %eax
  je   child

parent:
.align 8
	movl	eax,%eax
	movl	ebx,%ebx
	movl	ecx,%ecx
	movl	edx,%edx
	movl	esi,%esi
	movl	edi,%edi
	movl	ebp,%ebp
	movl	esp,%esp

loop:
	cmpl	eax,%eax
	jne	oops
	cmpl	ebx,%ebx
	jne	oops
	cmpl	ecx,%ecx
	jne	oops
	cmpl	edx,%edx
	jne	oops
	cmpl	esi,%esi
	jne	oops
	cmpl	edi,%edi
	jne	oops
	cmpl	ebp,%ebp
	jne	oops
	cmpl	esp,%esp
  jne oops
  jmp loop

child:
.align 8
  movl  eax2,%eax
  movl  ebx2,%ebx
  movl  ecx2,%ecx
  movl  edx2,%edx
  movl  esi2,%esi
  movl  edi2,%edi
  movl  ebp2,%ebp
  movl  esp2,%esp

loop2:
  cmpl  eax2,%eax
  jne   oops
  cmpl  ebx2,%ebx
  jne   oops
  cmpl  ecx2,%ecx
  jne   oops
  cmpl  edx2,%edx
  jne   oops
  cmpl  esi2,%esi
  jne   oops
  cmpl  edi2,%edi
  jne   oops
  cmpl  ebp2,%ebp
  jne   oops
  cmpl  esp2,%esp
  jne   oops
  jmp   loop2

oops:
	movl	%esp,newesp
	movl	oldesp,%esp
	movl	%ebp,newebp
	movl	oldesp,%ebp

	push	newebp
	push	newesp
	push	$edi
	push	$esi
	push	$edx
	push	$ecx
	push	$ebx
	push	$eax
	push	$test_name
	push	$test_pfx
	push	$str2
	call	lprintf

	push	$test_end_fail
	push	$test_name
	push	$test_pfx
	push	$str
	call	lprintf
	
	movl	oldesp,%esp
	movl	oldebp,%ebp
	movl	$0xffffffff,%eax
	ret
