#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	int i;

	for(i=1; i<argc; i++)
		if(i == 1) printf("%s", argv[i]);
		else printf(" %s", argv[i]);
	printf("\n");
	exit(0);
}
