/** @file 410user/progs/minclone_mem.S
 *  @author mpa
 *  @brief Tests sharing of memory in thread_fork
 *  @public yes
 *  @for p3
 *  @covers thread_fork
 *  @status needswork
 */
	
#include "410_tests.h"
#include "syscall_int.h"
	
.global main

test_name:
	.string "minclone_mem:"
str:	
	.string	"%s%s%s"
test_start:	
	.string TEST_START_CMPLT
test_pfx:
	.string TEST_PFX
test_end_success:
	.string TEST_END_SUCCESS
	
		
.comm	hpvar,4,4

main:
	pushl	%ebp
	movl	%esp,%ebp
	
	pushl	$test_start
	pushl	$test_name
	pushl	$test_pfx
	pushl	$str
	call	lprintf
	
	movl	$0x0,hpvar
	addl	$0x4,%esp
	movl	$0x0,4(%esp)
		
	/* fork a thread */
	int	$THREAD_FORK_INT
	
	cmpl	$0x0,%eax
	je	child

parent:		
	movl	$0x172,4(%esp)	/* signal child */
loop:	
	cmpl	$0xbacacaba,hpvar /* wait to be signaled */
	jne loop

	/* Ok, test was successful... */
	
	pushl	$test_end_success
	pushl	$test_name
	pushl	$test_pfx
	pushl	$str
	call	lprintf

	/* exit(0) */
	movl	$0x0,%esi
	int	$SET_STATUS_INT
	int	$VANISH_INT
	
	leave
	ret


child:
	cmpl	$0x172,4(%esp) /* wait to be signaled */
	jne	child

	movl	$0xbacacaba,hpvar /* signal parent */
	
	int	$VANISH_INT	/* bye-bye */
