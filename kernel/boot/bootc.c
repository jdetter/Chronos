#include "types.h"
#include "serial.h"
#include "stdlib.h"
#include "ata.h"
#include "stdlib.h"
#include "stdmem.h"
#include "console.h"
#include "keyboard.h"

#define KERNEL_LOAD 0x100000

/**
 * Stage 2 of the boot loader. This code must load the kernel from disk 
 * and then jump into the main method of the kernel.
 */

char* welcome = "Welcome to Chronos!\n";

int main(void)
{
	int serial = 1;
	if(serial_init(0))
	{
		/* There is no serial port. */
		serial = 0;
	}
	if(serial) serial_write(welcome, strlen(welcome));

	/* Initilize memory allocator. */
	minit(0x60A00, 0x70000, 0);

	cinit();
	cprintf("%s", welcome);

	/* Lets try to load the kernel */
	//uchar* kernel = (uchar*)KERNEL_LOAD;



	for(;;);
	return 0;
}
