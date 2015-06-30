#include "types.h"
#include "idt.h"
#include "trap.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "chronos.h"
#include "x86.h"
#include "panic.h"
#include "pic.h"
#include "pit.h"
#include "stdarg.h"
#include "stdlib.h"
#include "syscall.h"
#include "tty.h"
#include "fsman.h"
#include "pipe.h"
#include "proc.h"

#define TRAP_COUNT 256
#define INTERRUPT_TABLE_SIZE (sizeof(struct int_gate) * TRAP_COUNT)

struct int_gate interrupt_table[TRAP_COUNT];
extern struct proc* rproc;
extern uint trap_handlers[];
uint __get_cr2__(void);

void trap_return();

void trap_init(void)
{
	int x;
	for(x = 0;x < TRAP_COUNT;x++)
	{
		interrupt_table[x].offset_1 = (uint)(trap_handlers[x] & 0xFFFF);
		interrupt_table[x].offset_2 = 
			(trap_handlers[x] >> 16) & 0xFFFF;
		interrupt_table[x].segment_selector = SEG_KERNEL_CODE << 3;
		interrupt_table[x].flags = GATE_INT_CONST | GATE_USER;
	}

	lidt((uint)interrupt_table, INTERRUPT_TABLE_SIZE);		
}

void trap_handler(struct trap_frame* tf)
{
	//cprintf("Process %d has entered trap handler.\n", rproc->pid);
	rproc->tf = tf;
	int trap = tf->trap_number;
	char fault_string[64];
	int syscall_ret = -1;

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
			tty_print_string(rproc->t, "%s: Seg Fault: 0x%x\n",
				rproc->name, 
				__get_cr2__());
			sys_exit();
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
			//cprintf("System call started\n");
			strncpy(fault_string, "System Call", 64);
			syscall_ret = syscall_handler((uint*)tf->esp);
			rproc->tf->eax = syscall_ret;
			//cprintf("System call done.\n");
			//yield();
			break;
		case INT_PIC_TIMER: /* Time quantum has expired. */
			//cprintf("Timer interrupt!\n");
			pic_eoi(INT_PIC_TIMER_CODE);
			yield();
			break;
		default:
			strncpy(fault_string, "Invalid interrupt.", 64);
	}

        if(trap != TRAP_SC && trap != INT_PIC_TIMER)
        {
                cprintf("%s: 0x%x", fault_string, tf->error);
                for(;;);
        }

	/* Make sure that the interrupt flags is set */
	tf->eflags |= EFLAGS_IF;

	//cprintf("Process %d is leaving trap handler.\n", rproc->pid);

	/* While were here, clear the timer interrupt */
	// pic_eoi(INT_PIC_TIMER_CODE);

	/* Force return */
	asm volatile("movl %ebp, %esp");
	asm volatile("popl %ebp");
	/* add return address and arguments */
	asm volatile("addl $0x08, %esp");
	asm volatile("jmp trap_return");
}
