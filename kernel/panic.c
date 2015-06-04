#include "types.h"
#include "tty.h"
#include "stdarg.h"
#include "stdlib.h"

void panic(char* str)
{
	cprintf("Kernel panic: %s", str);
	for(;;);
}
