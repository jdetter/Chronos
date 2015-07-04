#include "types.h"
#include "stdarg.h"
#include "stdio.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "stdlib.h"

int main(int argc, char* argv[])
{
	int moct = 0;
	char* mode = NULL;

	int i;
	for(i = 1;i < argc; i++)
	{
		if(argv[i][0] != '-')
		{
			if(mode == NULL)
			{
				mode = argv[i];
				moct = atoi(mode, 8);
			} else {
				printf("%s: %d\n", argv[i], moct);
				chmod(argv[i], moct);	
			}
		}
	}

	exit();
}
