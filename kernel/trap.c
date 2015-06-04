#include "types.h"
#include "idt.h"
#include "trap.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "x86.h"
#include "panic.h"
#include "pic.h"
#include "serial.h"
#include "stdarg.h"
#include "stdlib.h"
#include "syscall.h"

#define TRAP_COUNT 256
#define INTERRUPT_TABLE_SIZE (sizeof(struct int_gate) * TRAP_COUNT)

struct int_gate interrupt_table[TRAP_COUNT];
extern uint trap_handlers[];

void trap_init(void)
{
	int x;
	for(x = 0;x < TRAP_COUNT;x++)
	{
		interrupt_table[x].offset_1 = (uint)(trap_handlers[x] & 0xFFFF);
		interrupt_table[x].offset_2 = 
			(trap_handlers[x] >> 16) & 0xFFFF;
		interrupt_table[x].segment_selector = SEG_KERNEL_CODE << 3;
		interrupt_table[x].flags = GATE_INT_CONST;
	}

	lidt((uint)interrupt_table, INTERRUPT_TABLE_SIZE);		
}

void trap_handler(struct trap_frame* tf)
{
	int trap = tf->trap_number;

	char fault_string[64];

	switch(trap)
	{
		case TRAP_DE:
			strncpy(fault_string, "Divide by 0", 64);
			break;
		case TRAP_DB:
			strncpy(fault_string, "Debug", 64);
			break;
		case TRAP_N1:
			strncpy(fault_string, "Non maskable interrupt", 64);
			break;
		case TRAP_BP:
			strncpy(fault_string, "Breakpoint", 64);
			break;
		case TRAP_OF:
			strncpy(fault_string, "Overflow", 64);
			break;
		case TRAP_BR:
			strncpy(fault_string, "Seg Fault", 64);
			break;
		case TRAP_UD:
			strncpy(fault_string, "Invalid Instruction", 64);
			break;
		case TRAP_NM:
			strncpy(fault_string, "Device not available", 64);
			break;
		case TRAP_DF:
			strncpy(fault_string, "Double Fault", 64);
			break;
		case TRAP_MF:
			strncpy(fault_string, "Coprocessor Seg fault", 64);
			break;
		case TRAP_TS:
			strncpy(fault_string, "Invalid TSS", 64);
			break;
		case TRAP_NP:
			strncpy(fault_string, "Invalid segment", 64);
			break;
		case TRAP_SS:
			strncpy(fault_string, "Stack Overflow", 64);
			break;
		case TRAP_GP:
			strncpy(fault_string, "Protection violation", 64);
			break;
		case TRAP_PF:
			strncpy(fault_string, "Page Fault", 64);
			break;
		case TRAP_0F:
			strncpy(fault_string, "Reserved interrupt", 64);
			break;
		case TRAP_FP:
			strncpy(fault_string, "Floating point error", 64);
			break;
		case TRAP_AC:
			strncpy(fault_string, "Bad alignment", 64);
			break;
		case TRAP_MC:
			strncpy(fault_string, "Machine Check", 64);
			break;
		case TRAP_XM:
			strncpy(fault_string, "SIMD Exception", 64);
			break;
		case TRAP_SC:
			strncpy(fault_string, "System Call", 64);
			syscall_handler((uint*)tf->esp);
			break;
		default:
			strncpy(fault_string, "Invalid interrupt.", 64);
	}

	serial_write(fault_string, strlen(fault_string));

	//pic_eoi(trap);
	for(;;);
}
