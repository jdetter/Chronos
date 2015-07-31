#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <chronos/types.h>

int main(int argc, char* argv[])
{
	int i;
	for(i=1; i<argc; i ++)
	{
		printf("%s\n", argv[i]);
		create(argv[i], PERM_ARD | PERM_UWR | PERM_GWR);
	}

	return 0;
}



