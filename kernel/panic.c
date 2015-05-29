#include "types.h"
#include "serial.h"
#include "console.h"

void panic(char* str)
{
	cprintf("%s", str);
	serial_write(str, sizeof(str));
	for(;;);
}
