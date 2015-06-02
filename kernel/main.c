#include "types.h"
#include "x86.h"
#include "pic.h"
#include "pit.h"
#include "stdlib.h"
#include "serial.h"
#include "stdmem.h"
#include "console.h"
#include "boot_pic.h"

char* loaded = "Welcome to the Chronos kernel!\n";

void main_stack();

/* Entry point for the kernel */
int main()
{
	/* WARNING: we don't have a proper stack right now. */
	serial_write(loaded, strlen(loaded));

	minit(0x00008000, 0x00070000, 0);
	cinit();
	//cprintf(loaded);	

	display_boot_pic();
	for(;;);

	/* Setup virtual memory and allocate a proper stack. */

	main_stack();
	/* main_stack doesn't return. */
	return 0;
}

void main_stack()
{
	/* We now have a proper kernel stack */
}
