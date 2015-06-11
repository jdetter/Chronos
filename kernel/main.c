#include "types.h"
#include "pic.h"
#include "pit.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdmem.h"
#include "boot_pic.h"
#include "tty.h"
#include "vsfs.h"
#include "stdlock.h"
#include "proc.h"
#include "vm.h"
#include "panic.h"
#include "idt.h"
#include "x86.h"

void __set_stack__(uint addr);
void main_stack(void);

extern struct proc* init_proc;
extern struct proc* rproc;
extern uint k_stack;

/* Entry point for the kernel */
int main(void)
{
	cprintf("Welcome to the Chronos kernel!\n");

	/* WARNING: we don't have a proper stack right now. */
	/* Get vm up */
	cprintf("Initilizing Virtual Memory...\t\t\t\t\t\t");
	vm_init();
	cprintf("[ OK ]\n");

	cprintf("Creating temporary stack...\t\t\t\t\t\t");
	/* Switch to a temporary stack */
	uint temp_stack = palloc();
	__set_stack__(temp_stack);
	cprintf("[ OK ]\n");

        /* Install global descriptor table */
        cprintf("Loading Global Descriptor table...\t\t\t\t\t");
        vm_seg_init();
	cprintf("[ OK ]\n");

        /* Enable paging */
        cprintf("Creating kernel stack...\t\t\t\t\t\t");
	switch_kvm();

	/* Setup proper stack */
	uint new_stack = KVM_KSTACK_E;
	k_stack = new_stack;

	__set_stack__(new_stack);
	/* Stability: Do not add any instructions here! (unstable stack)*/
	main_stack();

	panic("main_stack returned.\n");
	for(;;);
	return 0;
}

/* Proper 4K stack*/
void main_stack(void)
{
	cprintf("[ OK ]\n");
	/* We now have a proper kernel stack */

	/* Start disk driver */
	cprintf("Starting disk driver...\t\t\t\t\t\t\t");
	vsfs_init(64);
	cprintf("[ OK ]\n");
	
	/* Bring up kmalloc. */
        cprintf("Initilizing kmalloc...\t\t\t\t\t\t\t");
	minit(KVM_KMALLOC_S, KVM_KMALLOC_E);
	cprintf("[ OK ]\n");

	/* Install interrupt descriptor table */
	cprintf("Installing Interrupt Descriptor table...\t\t\t\t");
	trap_init();
	cprintf("[ OK ]\n");

	/* Enable PIC */
        cprintf("Starting Programmable Interrupt Controller Driver...\t\t\t");
	pic_init();
	cprintf("[ OK ]\n");
	
	/* Enable PIT */
        //pit_init();

	/* Enable interrupts */
	cprintf("Enabling Interrupts...\t\t\t\t\t\t\t");
	asm volatile("sti");	
	cprintf("[ OK ]\n");

	cprintf("Initilizing Process Scheduler...\t\t\t\t\t");
	sched_init();
	cprintf("[ OK ]\n");
	
	cprintf("Spawning tty0...\t\t\t\t\t\t\t\t");
	/* Setup an init process */
	init_proc = spawn_tty(tty_find(0));
	cprintf("[ OK ]\n");
	
	/* Start scheduling loop. */
	sched();

	//rproc = p;
	//switch_context(p);
	
	for(;;);
}

void msetup()
{
	panic("Memory allocator not initilized.");
}
