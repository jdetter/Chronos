#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	open("/bin", O_RDONLY);
	printf("Leaking a file...\n");
	_exit(1);

	exit(0);
}
