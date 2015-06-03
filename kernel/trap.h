#ifndef _TRAP_H_
#define _TRAP_H_

/**
 * A trap frame is what gets pushed onto the stack during an interrupt. This
 * context should be restored when the process wants to get rescheduled.
 */
struct trap_frame
{
	/* 32 bit Registers from pushad */
	uint_32 edi;
	uint_32 esi;
	uint_32 ebp;
	uint_32 espx; /* This gets restored, but overridden by esp below */
	uint_32 ebx;
	uint_32 edx;
	uint_32 ecx;
	uint_32 eax;

	/* Segment registers */
	uint_16 gs;
	uint_16 padding1;
	uint_16 fs;
	uint_16 padding2;
	uint_16 es;
	uint_16 padding3;
	uint_16 ds;
	uint_16 padding4;

	/* Trap number pushed by trap handlers */
	uint_32 trap_number;

	/* Pushed during the interrupt */
	uint_32 error;
	uint_32 eip; /* Instruction Pointer */
	uint_16 cs; /* Code segment selector */
	uint_16 padding5; /* 16 bit padding */
	uint_32 eflags; /* Status bits for this process. */

	/* Pushed if there was a privilege change */
	uint_32 esp; /* The stack pointer that will be restored. (see espx) */
	uint_16 ss; /* Stack segment selector. */
	uint_16 padding6;
};

#endif
