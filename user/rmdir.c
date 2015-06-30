#include "types.h"
#include "stdarg.h"
#include "stdio.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"

int main(int argc, char* argv[])
{
	int i;
	for(i=1; i<argc; i ++)
	{
		printf("%s\n", argv[i]);
		if(rmdir(argv[i]))
		{
			printf("FAIL\n");
		}
	}
	exit();
}
