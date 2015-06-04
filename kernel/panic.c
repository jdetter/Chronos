#include "types.h"
#include "serial.h"
#include "console.h"
#include "stdarg.h"
#include "stdlib.h"

char* panic_str = "Kernel panic: ";

void panic(char* str)
{
	//cprintf("%s", panic_str);
	//cprintf("%s", str);
	serial_write(panic_str, strlen(panic_str));
	serial_write(str, strlen(str));
	for(;;);
}
