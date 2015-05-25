#include "types.h"
#include "serial.h"
#include "stdlib.h"

/**
 * Stage 2 of the boot loader. This code must load the kernel from disk 
 * and then jump into the main method of the kernel.
 */

int main(void)
{
	serial_init(0);
	char* buffer = "Remember, no Russian.\n";

	serial_write(buffer, strlen(buffer));

	for(;;);

	return 0;
}
