#include "types.h"
#include "pic.h"
#include "pit.h"
#include "stdlib.h"
#include "serial.h"
#include "stdmem.h"
#include "console.h"
#include "boot_pic.h"
#include "tty.h"
#include "vsfs.h"
#include "stdlock.h"
#include "proc.h"
#include "vm.h"
#include "panic.h"
#include "idt.h"
#include "x86.h"

char* loaded = "Welcome to the Chronos kernel!\n";
char* str_vm = "Initilizing Virtual Memory...\t\t\t\t\t\t";
char* str_stack = "Creating kernel stack...\t\t\t\t\t\t";
char* str_gdt = "Loading Global Descriptor table...\t\t\t\t\t";
char* str_kmalloc = "Initilizing kmalloc...\t\t\t\t\t\t\t";
char* str_display = "Starting display driver...\t\t\t\t\t\t";
char* str_idt = "Installing Interrupt Descriptor table...\t\t\t\t";
char* str_pic = "Starting Programmable Interrupt Controller Driver...\t\t\t";
char* str_pit = "Starting Programmable Interrupt Timer Driver...\t\t\t\t";
char* str_int = "Enabling Interrupts...\t\t\t\t\t\t\t";
char* str_paging = "Enabling kernel paging...\t\t\t\t\t\t";

char* str_ok = "[ OK ]\n";
char* str_fail = "[FAIL]\n";

extern struct proc* rproc;
void fake_trap(void);


void __set_stack__(uint addr);
void main_stack(void);

extern uint k_stack;

/* Entry point for the kernel */
int main(void)
{
	serial_write(loaded, strlen(loaded));

	/* WARNING: we don't have a proper stack right now. */
	/* Get vm up */
	serial_write(str_vm, strlen(str_vm));
	vm_init();
	serial_write(str_ok, strlen(str_ok));

	/* Setup proper stack */
	serial_write(str_stack, strlen(str_stack));
	uint new_stack = palloc();
	new_stack += PGSIZE;
	__set_stack__(new_stack);
	k_stack = new_stack;

	main_stack();
	serial_write(str_fail, strlen(str_fail));
	panic("main_stack returned.\n");
	for(;;);
	return 0;
}

/* Proper 4K stack*/
void main_stack(void)
{
	serial_write(str_ok, strlen(str_ok));
	/* We now have a proper kernel stack */

	/* Install global descriptor table */
	serial_write(str_gdt, strlen(str_gdt));
        vm_seg_init();
	serial_write(str_ok, strlen(str_ok));

	serial_write(str_paging, strlen(str_paging));
	switch_kvm();
	serial_write(str_ok, strlen(str_ok));
	
	/* Bring up kmalloc. */
	serial_write(str_kmalloc, strlen(str_kmalloc));
        minit(KVM_KMALLOC_S, KVM_KMALLOC_E, 0);
	serial_write(str_ok, strlen(str_ok));

	/* Install interrupt descriptor table */
	serial_write(str_idt, strlen(str_idt));
	trap_init();
	serial_write(str_ok, strlen(str_ok));

	/* Enable PIC */
        serial_write(str_pic, strlen(str_pic));
        pic_init();
        serial_write(str_ok, strlen(str_ok));
	
	/* Enable PIT */
        //serial_write(str_pit, strlen(str_pit));
        //pit_init();
        //serial_write(str_ok, strlen(str_ok));

	serial_write(str_int, strlen(str_int));
	asm volatile("sti");	
	serial_write(str_ok, strlen(str_ok));

	/* Setup a fake process */
	struct proc p;
	memset(&p, 0, sizeof(struct proc));
	p.stack_start = k_stack;
	p.stack_end = k_stack + PGSIZE;
	rproc = &p;

	fake_trap();
	
	/* Force an interrupt */
	for(;;);
}
