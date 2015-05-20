#include "types.h"
#include "serial.h"
#include "stdlib.h"
#include "ata.h"

/**
 * Stage 2 of the boot loader. This code must load the kernel from disk 
 * and then jump into the main method of the kernel.
 */

char* welcome = "Welcome to Chronos!\n";
char* disk = "Writing to disk...\n";
char* done = "Writing to disk complete.\n";

int main(void)
{
	serial_init(0);
	serial_write(welcome, strlen(welcome));

	uchar buffer[512];
	int x;
	for(x = 0;x < 512;x++)
		buffer[x] = x;

	serial_write(disk, strlen(disk));
	ata_writesect(0, buffer);

	serial_write(done, strlen(done));
	
	for(;;);
	return 0;
}
