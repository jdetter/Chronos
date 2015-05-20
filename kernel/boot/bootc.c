#include "types.h"
#include "serial.h"
#include "stdlib.h"
#include "ata.h"

/**
 * Stage 2 of the boot loader. This code must load the kernel from disk 
 * and then jump into the main method of the kernel.
 */

char* welcome = "Welcome to Chronos!\n";
char* prompt = "Enter text: \n";
char* done = "Done listening.\n";

int main(void)
{
	serial_init(0);
	serial_write(welcome, strlen(welcome));
	serial_write(prompt, strlen(prompt));

	char buffer[1000];

	serial_read(buffer, 10);
	
	serial_write(done, strlen(done));
	serial_write(buffer, strlen(buffer));

	for(;;);
	return 0;
}
