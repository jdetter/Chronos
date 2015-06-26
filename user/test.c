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
	int fds[2]
	if(pipe(fds))
	{
		printf("Couldn't create pipe!!\n");
		exit();
	}

	printf("fd read:  %d\n", fds[0]);
	printf("fd write: %d\n", fds[1]);

	exit();
}
