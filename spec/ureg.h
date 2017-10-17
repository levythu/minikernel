#ifndef _UREG_H_
#define _UREG_H_

/* See intel-sys.pdf section 5.12, "Exception and Interrupt Reference" */
#define SWEXN_CAUSE_DIVIDE       0x00  /* Very clever, Intel */
#define SWEXN_CAUSE_DEBUG        0x01
#define SWEXN_CAUSE_BREAKPOINT   0x03
#define SWEXN_CAUSE_OVERFLOW     0x04
#define SWEXN_CAUSE_BOUNDCHECK   0x05
#define SWEXN_CAUSE_OPCODE       0x06  /* SIGILL */
#define SWEXN_CAUSE_NOFPU        0x07  /* FPU missing/disabled/busy */
#define SWEXN_CAUSE_SEGFAULT     0x0B  /* segment not present */
#define SWEXN_CAUSE_STACKFAULT   0x0C  /* ouch */
#define SWEXN_CAUSE_PROTFAULT    0x0D  /* aka GPF */
#define SWEXN_CAUSE_PAGEFAULT    0x0E  /* cr2 is valid! */
#define SWEXN_CAUSE_FPUFAULT     0x10  /* old x87 FPU is angry */
#define SWEXN_CAUSE_ALIGNFAULT   0x11
#define SWEXN_CAUSE_SIMDFAULT    0x13  /* SSE/SSE2 FPU is angry */

#ifndef ASSEMBLER

typedef struct ureg_t {
	unsigned int cause;  // 0
	unsigned int cr2;   /* Or else zero. */  // 4

	unsigned int ds; // 8
	unsigned int es; // 12
	unsigned int fs; // 16
	unsigned int gs; // 20

	unsigned int edi;  // 24
	unsigned int esi;  // 28
	unsigned int ebp;  // 32
	unsigned int zero;  /* Dummy %esp, set to zero */  // 36
	unsigned int ebx;  // 40
	unsigned int edx;  // 44
	unsigned int ecx;  // 48
	unsigned int eax;  // 52

	unsigned int error_code; // 56
	unsigned int eip;  // 60
	unsigned int cs;   // 64
	unsigned int eflags; // 68
	unsigned int esp;  // 72
	unsigned int ss;   // 76
} ureg_t;

#endif /* ASSEMBLER */

#endif /* _UREG_H_ */
