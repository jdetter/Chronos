#include "types.h"
#include "x86.h"
#include "pic.h"
#include "pit.h"
#include "stdlib.h"
#include "serial.h"
#include "stdmem.h"
#include "console.h"
#include "boot_pic.h"
#include "vm.h"
#include "panic.h"

char* loaded = "Welcome to the Chronos kernel!\n";
char* vm_start = "Initilizing Virtual Memory...\n";
char* seg_start = "Installing new descriptor table...\n";

void __set_stack__(uint addr);
void main_stack();

/* Entry point for the kernel */
int main()
{
	/* WARNING: we don't have a proper stack right now. */
	/* Get vm up */
	serial_write(vm_start, strlen(vm_start));
	vm_init(KVM_MALLOC, 0x00EFFFFF);
	uint new_stack = palloc();
	new_stack += PGSIZE;
	__set_stack__(new_stack);	

	main_stack();
	panic("main_stack returned.\n");
	for(;;);
	return 0;
}

/* Proper 4K stack*/
void main_stack()
{
	/* We now have a proper kernel stack */
        serial_write(seg_start, strlen(seg_start));
        vm_seg_init();

        serial_write(loaded, strlen(loaded));
        minit(0x00008000, 0x00070000, 0);
        cinit();
        display_boot_pic();
}
