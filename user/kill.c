#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		printf("Usage: kill <pid>\n");
		return -1;
	}

	int pid = atoi(argv[1]);
	kill(pid, SIGKILL);

	return 0;
}
