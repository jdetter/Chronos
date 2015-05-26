#include "types.h"
#include "serial.h"
#include "stdlib.h"
#include "ata.h"
#include "stdlib.h"
#include "stdmem.h"
#include "console.h"

/**
 * Stage 2 of the boot loader. This code must load the kernel from disk 
 * and then jump into the main method of the kernel.
 */

char* welcome = "Welcome to Chronos!\n";

int main(void)
{
	//serial_init(0);
	//serial_write(welcome, strlen(welcome));

	/* Initilize memory allocator. */
	minit(0x110000, 0x120000, 0);

	cinit();
	cprintf("Welcome to Chronos!\n");
	cprintf("New line.\n");

	for(;;);
	return 0;
}
