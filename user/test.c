#include "types.h"
#include "stdio.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdmem.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"

int main(int argc, char** argv)
{
	proc_dump();
	int result = rm("/boot/chronos.elf");
	printf("result: %d\n", result);
	proc_dump();

	exit(0);
}
