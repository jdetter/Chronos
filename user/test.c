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
	open("/bin", O_RDONLY);
	printf("Leaking a file...\n");
	_exit(1);

	exit(0);
}
