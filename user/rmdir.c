#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	int i;
	for(i=1; i<argc; i ++)
	{
		printf("%s\n", argv[i]);
		rmdir(argv[i]);
	}
	exit(0);
}
