/**
 * Authors: 
 *  + John Detter <john@detter.com>
 *  + Amber Arnold <alarnold2@wisc.edu>
 *
 * Trap handling functions.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kstdlib.h"
#include "idt.h"
#include "trap.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "chronos.h"
#include "x86.h"
#include "panic.h"
#include "stdarg.h"
#include "syscall.h"
#include "tty.h"
#include "fsman.h"
#include "pipe.h"
#include "proc.h"
#include "signal.h"
#include "vm.h"
#include "k/vm.h"
#include "drivers/pic.h"
#include "drivers/pit.h"
#include "drivers/cmos.h"
#include "drivers/rtc.h"

#define TRAP_COUNT 256
#define INTERRUPT_TABLE_SIZE (sizeof(struct int_gate) * TRAP_COUNT)
#define STACK_TOLERANCE 16

// #define DEBUG
// #define PANIC_ON_ANY_FAULT

#ifdef DEBUG
#define TRAP_NAME_SZ 20
char* trap_names[] = {
        "DIVIDE ERROR",
        "DEBUG",
        "NON MASKABLE INTERRUPT",
        "BREAK RECEIVED",
        "OVERFLOW CAUGHT",
        "BOUND RANGE EXCEEDED",
        "INVALID OPCODE",
        "DEVICE NOT AVAILABLE",
        "DOUBLE FAULT",
        "COPROCESSOR SEGMENT OVERRUN",
        "INVALID TSS",
        "SEGMENT NOT PRESENT",
        "STACK SEGMENT FAULT",
        "GENERAL PROTECTION FAULT",
        "PAGE FAULT",
        "RESERVED INTERRUPT",
        "FLOATING POINT EXCEPTION",
        "ALIGNMENT CHECK",
        "MACHINE CHECK",
        "SIMD FLOATING POINT EXCEPTION"
};
#endif

extern struct rtc_t k_time;
struct int_gate interrupt_table[TRAP_COUNT];
extern uint trap_handlers[];
extern int k_ticks;

void trap_init(void)
{
	/* Initilize the trap table */
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

int trap_pf(uintptr_t address)
{
#ifdef DEBUG
	cprintf("Fault address: 0x%x\n", address);
#endif
	vmflags_t dir_flags = VM_DIR_USRP | VM_DIR_READ | VM_DIR_WRIT;
	vmflags_t tbl_flags = VM_TBL_USRP | VM_TBL_READ | VM_TBL_WRIT;

	uintptr_t stack_bottom = rproc->stack_end;
	uintptr_t stack_tolerance = stack_bottom - STACK_TOLERANCE * PGSIZE;

	if(address < stack_bottom && address >= stack_tolerance){
		uintptr_t address_down = PGROUNDDOWN(address);
		if(address_down <= PGROUNDUP(rproc->heap_end))
			return 1;

		int numOfPgs = (stack_bottom - address_down) / PGSIZE;
		vm_mappages(address_down, numOfPgs * PGSIZE, rproc->pgdir, 
			dir_flags, tbl_flags);

		/* Move the stack end */
		rproc->stack_end -= numOfPgs * PGSIZE;
	} else {
#ifdef __ALLOW_VM_SHARE__
		/* Is this copy on write? */
		if(vm_is_cow(rproc->pgdir, address))
		{
			vm_uncow(rproc->pgdir, address);
			return 0;
		}
#endif
		return 1;
	}

	return 0;
}

void trap_handler(struct trap_frame* tf, void* ret_frame)
{
	rproc->tf = tf;
	
	int trap = tf->trap_number;
	char fault_string[64];
	int syscall_ret = -1;
	int handled = 0;
	int user_problem = 0;
	int kernel_fault = 0;

#ifdef DEBUG
	/* Save the current eip for testing */	
	uintptr_t ret_eip = tf->eip;
#endif

	if(!(tf->cs & 0x3))
		kernel_fault = 1;

	/**
	 * A couple of quick optimizations:
	 *  + Clock interrupt is the most common interrupt
	 *  + System call is very common interrupt
	 *  + signal handling shouldn't handle in switch
	 */

#ifdef DEBUG
	if(trap < TRAP_NAME_SZ)
		cprintf("trap: %s\n", trap_names[trap]);
#endif

	handled = 1;
	if(trap == TRAP_SC)
	{
#ifdef DEBUG
		cprintf("trap: SYSTEM CALL\n");
#endif
		strncpy(fault_string, "System Call", 64);
		syscall_ret = syscall_handler((int*)tf->esp);
		rproc->tf->eax = syscall_ret;
	} else if(trap == INT_PIC_TIMER)
	{
		rproc->user_ticks++;
		k_ticks++;
		pic_eoi(INT_PIC_TIMER_CODE);
		yield();
	} else if(tf->eip == SIG_MAGIC && rproc->sig_handling)
	{
		/* We're done handling this signal! */
		sig_cleanup();
		rproc->sig_handling = 0;
	} else handled = 0; /* Its not any of the above optimizations */

	if(handled) goto TRAP_DONE;

	switch(trap)
	{
		case INT_PIC_KEYBOARD: case INT_PIC_COM1:
			handled = 1;
			break;
			// cprintf("Keyboard interrupt.\n");
			/* Keyboard interrupt */
			tty_keyboard_interrupt_handler();
			pic_eoi(INT_PIC_TIMER_CODE);
			pic_eoi(INT_PIC_COM1_CODE);
			handled = 1;
			break;
		case INT_PIC_CMOS:
			/* Update the system time */
			pic_eoi(INT_PIC_CMOS_CODE);
			uchar cmos_val = cmos_read_interrupt();
			if(cmos_val == 144)
			{
				/* Clock update */
				rtc_update(&k_time);
			}
			handled = 1;
			break;
		case TRAP_DE:
			strncpy(fault_string, "Divide by 0", 64);
			user_problem = 1;
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
			user_problem = 1;
			break;
		case TRAP_BR:
			strncpy(fault_string, "Seg Fault", 64);
			user_problem = 1;
			break;
		case TRAP_UD:
			strncpy(fault_string, "Invalid Instruction", 64);
			user_problem = 1;
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
			user_problem = 1;
			break;
		case TRAP_GP:
			strncpy(fault_string, "Protection violation", 64);
			user_problem = 1;
			break;
		case TRAP_PF:
			if(kernel_fault) 
			{
				strncpy(fault_string, "Seg Fault", 64);
                                tf->error = vm_get_page_fault_address();
				break;
			}

			if(trap_pf(vm_get_page_fault_address())){
				strncpy(fault_string, "Seg Fault", 64);
				tf->error = vm_get_page_fault_address();
				user_problem = 1;
			}else{
				handled = 1;
			}
			break;
		case TRAP_0F:
			strncpy(fault_string, "Reserved interrupt", 64);
			break;
		case TRAP_FP:
			strncpy(fault_string, "Floating point error", 64);
			user_problem = 1;
			break;
		case TRAP_AC:
			strncpy(fault_string, "Bad alignment", 64);
			user_problem = 1;
			break;
		case TRAP_MC:
			strncpy(fault_string, "Machine Check", 64);
			user_problem = 1;
			break;
		case TRAP_XM:
			strncpy(fault_string, "SIMD Exception", 64);
			break;
		default:
			cprintf("Interrupt number: %d\n", trap);
			strncpy(fault_string, "Invalid interrupt.", 64);
	}

TRAP_DONE:

#ifdef DEBUG
	if(ret_eip != rproc->tf->eip)
	{
		cprintf("%s:%d: WARNING: eip has changed!!\n",
			rproc->name, rproc->pid);
		cprintf("\t\t0x%x --> 0x%x\n",
			ret_eip, rproc->tf->eip);
	}
#endif

	if(!handled)
	{
		cprintf("%s: 0x%x", fault_string, tf->error);
		cprintf("%s: EIP: 0x%x ESP: 0x%x EBP: 0x%x\n",
			rproc->name, rproc->tf->eip,
			rproc->tf->esp, rproc->tf->ebp);

#ifdef PANIC_ON_ANY_FAULT
		user_problem = 0;
#endif

		if(user_problem && rproc && !kernel_fault)
		{
			_exit(1);
		} else {
			cprintf("\n\nKERNEL FAULT\n");
			/* Write everything to disk so logs can be extracted */
			fs_sync();
			// cprintf("chronos: kernel panic\n");
			for(;;);
		}
	}


	/* Do we have any signals waiting? */
	if(rproc->sig_queue && !rproc->sig_handling)
	{
		slock_acquire(&ptable_lock);
		cprintf("Process %s is handling signal...\n", rproc->name);
		sig_handle();
		slock_release(&ptable_lock);
	}


	/* Make sure that the interrupt flags is set */
	tf->eflags |= EFLAGS_IF;

	//cprintf("Process %d is leaving trap handler.\n", rproc->pid);

	/* While were here, clear the timer interrupt */
	// pic_eoi(INT_PIC_TIMER_CODE);

	/* Warning: Black magic below, this will be fixed later */
	/* Force return */
	asm volatile("movl %ebp, %esp");
	asm volatile("popl %ebp");
	/* add return address */
	asm volatile("addl $0x08, %esp");
	asm volatile("jmp tp_trap_return");
}
