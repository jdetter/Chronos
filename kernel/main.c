#include "types.h"
#include "file.h"
#include "pic.h"
#include "pit.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdmem.h"
#include "boot_pic.h"
#include "fsman.h"
#include "stdlock.h"
#include "devman.h"
#include "tty.h"
#include "proc.h"
#include "vm.h"
#include "panic.h"
#include "idt.h"
#include "x86.h"

void __set_stack__(uint stack, uint function);
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

        /* Install global descriptor table */
        cprintf("Loading Global Descriptor table...\t\t\t\t\t");
        vm_seg_init();
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
	cprintf("Detecting devices...\t\t\t\t\t\t\t");
	dev_init();
	cprintf("[ OK ]\n");

	/* Start disk driver */
	cprintf("Starting file system manager...\t\t\t\t\t\t");
	fsman_init();
	cprintf("[ OK ]\n");
	
	/* Bring up kmalloc. */
        cprintf("Initilizing kmalloc...\t\t\t\t\t\t\t");
	minit(KVM_KMALLOC_S, KVM_KMALLOC_E);
	cprintf("[ OK ]\n");
	/* Enable memory debugging */
	//mem_debug((void (*)(char*))cprintf);

	/* Install interrupt descriptor table */
	cprintf("Installing Interrupt Descriptor table...\t\t\t\t");
	trap_init();
	cprintf("[ OK ]\n");

	/* Enable PIC */
        cprintf("Starting Programmable Interrupt Controller Driver...\t\t\t");
	pic_init();
	cprintf("[ OK ]\n");
	
	/* Enable PIT */
        cprintf("Starting Programmable Interrupt Timer Driver...\t\t\t\t");
	pit_init();
	cprintf("[ OK ]\n");

	/* Enable interrupts */
	cprintf("Enabling Interrupts...\t\t\t\t\t\t\t");
	//asm volatile("sti");	
	cprintf("[ OK ]\n");

	cprintf("Initilizing Process Scheduler...\t\t\t\t\t");
	sched_init();
	cprintf("[ OK ]\n");
	
	cprintf("Spawning tty0...\t\t\t\t\t\t\t");
	/* Setup an init process */
	init_proc = spawn_tty(tty_find(0));
	cprintf("[ OK ]\n");

	//cprintf("Spawning tty1...\t\t\t\t\t\t\t\t");
	//tty_t x = tty_find(1);
	//tty_init(x, 1, TTY_TYPE_COLOR, 1, KVM_COLOR_START);
	//tty_enable(x);
	//spawn_tty(x);
	//cprintf("[ OK ]\n");
	
	/* Start scheduling loop. */
	sched();
	panic("Scheduler returned!");
}

void msetup()
{
	panic("Memory allocator not initilized.");
}
