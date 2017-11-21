.globl hv_magic
.globl hv_exit
.globl hv_setidt
.globl hv_disable_interrupts
.globl hv_enable_interrupts
.globl hv_setpd
.globl hv_adjustpg
.globl hv_print
.globl hv_cons_set_term_color
.globl hv_cons_set_cursor_pos
.globl hv_cons_get_cursor_pos
.globl hv_iret
.globl hv_print_at

hv_magic:
hv_exit:
hv_setidt:
hv_disable_interrupts:
hv_enable_interrupts:
hv_setpd:
hv_adjustpg:
hv_print:
hv_cons_set_term_color:
hv_cons_set_cursor_pos:
hv_cons_get_cursor_pos:
hv_iret:
hv_print_at:
	pushl	%ebp
	movl	%esp, %ebp
	movl	$80808080, %eax
	movl	%eax, (%eax)
	leave
	ret
