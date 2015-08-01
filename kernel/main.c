#include "types.h"
#include "file.h"
#include "pic.h"
#include "pit.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdmem.h"
#include "stdlock.h"
#include "fsman.h"
#include "devman.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "cmos.h"
#include "panic.h"
#include "idt.h"
#include "x86.h"
#include "cpu.h"
#include "keyboard.h"
#include "rtc.h"
#include "ktime.h"
#include "trap.h"
#include "signal.h"

void __set_stack__(uint stack, uint function);
void main_stack(void);

extern struct rtc_t k_time;
extern struct proc* init_proc;
extern struct proc* rproc;
extern uint k_stack;

/* Entry point for the kernel */
int main(void)
{
	/* Lets make sure that tty0 gets configured properly. */
	setup_kvm();
	cprintf("Welcome to the Chronos kernel!\n");

	/* Install global descriptor table */
        cprintf("Loading Global Descriptor table...\t\t\t\t\t");
        vm_seg_init();
        cprintf("[ OK ]\n");

	/* Get interrupt descriptor table up in case of a crash. */
	/* Install interrupt descriptor table */
        cprintf("Installing Interrupt Descriptor table...\t\t\t\t");
        trap_init();
        cprintf("[ OK ]\n");
	
	/* WARNING: we don't have a proper stack right now. */
	/* Get vm up */
	cprintf("Initilizing Virtual Memory...\t\t\t\t\t\t");
	vm_init();
	cprintf("[ OK ]\n");

	/* Setup proper stack */
	uint new_stack = KVM_KSTACK_E;
	k_stack = new_stack;

	cprintf("Switching over to kernel stack...\t\t\t\t\t");
	__set_stack__(PGROUNDUP(new_stack), (uint)main_stack);

	panic("main_stack returned.\n");
	for(;;);
	return 0;
}

/* Proper 4K stack*/
void main_stack(void)
{
	/* We now have a proper kernel stack */
	cprintf("[ OK ]\n");

	/* Initilize signals */
	cprintf("Initilizing signals...\t\t\t\t\t\t\t");
	sig_init();
	cprintf("[ OK ]\n");
	
	/* Enable PIC */
        cprintf("Starting Programmable Interrupt Controller Driver...\t\t\t");
        pic_init();
        cprintf("[ OK ]\n");

        /* Initilize CMOS */
        cprintf("Initilizing cmos...\t\t\t\t\t\t\t");
        cmos_init();
        cprintf("[ OK ]\n");

	/* Initilize kernel time */
	cprintf("Initilizing kernel time...\t\t\t\t\t\t");
	ktime_init();
	cprintf("[ OK ]\n");

	/* Initilize keyboard driver */
	cprintf("Initilizing keyboard driver...\t\t\t\t\t\t");
	kbd_init();
	cprintf("[ OK ]\n");

	/* Detect devices */
	cprintf("Detecting devices...\t\t\t\t\t\t\t");
	dev_init();
	cprintf("[ OK ]\n");

	/* Start disk driver */
	cprintf("Starting file system manager...\t\t\t\t\t\t");
	fsman_init();
	cprintf("[ OK ]\n");

	/* Populate the device folder */
	cprintf("Populating the dev folder...\t\t\t\t\t\t");
	dev_populate();
	cprintf("[ OK ]\n");

	/* Enable PIT */
        cprintf("Starting Programmable Interrupt Timer Driver...\t\t\t\t");
	pit_init();
	cprintf("[ OK ]\n");

	/* Initilize cli stack */
	cprintf("Initilizing cli stack...\t\t\t\t\t\t");
	reset_cli();
	cprintf("[ OK ]\n");

	/* Initilize pipes */
	cprintf("Initilizing pipes...\t\t\t\t\t\t\t");
	pipe_init();
	cprintf("[ OK ]\n");
	
	cprintf("Initilizing Process Scheduler...\t\t\t\t\t");
	sched_init();
	cprintf("[ OK ]\n");
	
	cprintf("Spawning tty0...\t\t\t\t\t\t\t");
	/* Setup an init process */
	init_proc = spawn_tty(tty_find(0));
	cprintf("[ OK ]\n");

	/* Start scheduling loop. */
	sched();
	panic("Scheduler returned!");
}

void msetup()
{
	panic("kernel: Memory allocator not initilized.\n");
}

int mpanic(int size)
{
	panic("kernel: Memory allocator out of space.\n");
	return -1;
}
