#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#define __LINUX__
#include "file.h"

int main(int argc, char* argv[])
{
	int i;
	for(i=1; i<argc; i ++)
	{
		if(mkdir(argv[i], PERM_ARD | PERM_AEX
			| PERM_UWR | PERM_GWR))
		{
			printf("Failed to create directory: %s\n", argv[i]);
		} else printf("%s\n", argv[i]);
	}
	return 0;
}
