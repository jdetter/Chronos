#ifndef _ARCH_TRAP_H_
#define _ARCH_TRAP_H_

/* Segment descriptions */
/* Null segment         0x00 */
#define SEG_KERNEL_CODE 0x01
#define SEG_KERNEL_DATA 0x02
#define SEG_USER_CODE   0x03 /* data must be 1 away from code (sysexit) */
#define SEG_USER_DATA   0x04
#define SEG_TSS         0x05
#define SEG_COUNT       0x06

#define SYS_EXIT_BASE   ((SEG_USER_CODE << 3) - 16)

#ifndef __ASM_ONLY__

#include "k/trap.h"

#define TF_REGISTERS (0x08 << 2)
typedef char fpu128_t[16];

struct trap_frame
{
	/* Floating point context */
	fpu128_t fpu_w0;
	fpu128_t fpu_w1;

	fpu128_t fpu_mm0;
	fpu128_t fpu_mm1;
	fpu128_t fpu_mm2;
	fpu128_t fpu_mm3;
	fpu128_t fpu_mm4;
	fpu128_t fpu_mm5;
	fpu128_t fpu_mm6;
	fpu128_t fpu_mm7;

	fpu128_t fpu_xmm0;
	fpu128_t fpu_xmm1;
	fpu128_t fpu_xmm2;
	fpu128_t fpu_xmm3;
	fpu128_t fpu_xmm4;
	fpu128_t fpu_xmm5;
	fpu128_t fpu_xmm6;
	fpu128_t fpu_xmm7;

	fpu128_t fpu_resw0;
	fpu128_t fpu_resw1;
	fpu128_t fpu_resw2;
	fpu128_t fpu_resw3;
	fpu128_t fpu_resw4;
	fpu128_t fpu_resw5;
	fpu128_t fpu_resw6;
	fpu128_t fpu_resw7;
	fpu128_t fpu_resw8;
	fpu128_t fpu_resw9;
	fpu128_t fpu_resw10;

	fpu128_t fpu_avail0;
	fpu128_t fpu_avail1;
	fpu128_t fpu_avail2;

	/* Alignment for fpu */
	char fpu_alignment[436];

	/* 32 bit Registers from pushad */
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t espx; /* This gets restored, but overridden by esp below */
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	/* Segment registers */
	uint16_t gs;
	uint16_t padding1;
	uint16_t fs;
	uint16_t padding2;
	uint16_t es;
	uint16_t padding3;
	uint16_t ds;
	uint16_t padding4;

	/* Trap number pushed by trap handlers */
	uint32_t trap_number;

	/* Pushed during the interrupt */
	uint32_t error;
	uint32_t eip; /* Instruction Pointer */
	uint16_t cs; /* Code segment selector */
	uint16_t padding5; /* 16 bit padding */
	uint32_t eflags; /* Status bits for this process. */

	/* Pushed if there was a privilege change */
	uint32_t esp; /* The stack pointer that will be restored. (see espx) */
	uint16_t ss; /* Stack segment selector. */
	uint16_t padding6;
};

/**
 * Return into the user's context from a trap.
 */
void tp_trap_return(void) __attribute__ ((noreturn));

/**
 * Make a trap frame and jump into the trap handler.
 */
void tp_mktf(void) __attribute__ ((noreturn));

#endif

#endif
