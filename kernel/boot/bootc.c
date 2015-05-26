#include "types.h"
#include "serial.h"
#include "stdlib.h"
#include "ata.h"
#include "stdlib.h"
#include "stdmem.h"
#include "console.h"
#include "keyboard.h"

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

	cprintf("Number: %c %d\n", 'G', 100);

	//kbd_init();
	char c;
	for(c = 0;!c;)
	{
		c = kbd_getc();
	}
	cprintf("Character: %c\n", c);
	cprintf("Character: %c\n", 'a');

	for(;;);
	return 0;
}
