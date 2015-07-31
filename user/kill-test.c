#include "chronos.h"
#include "stdio.h"
#include "stdlib.h"

int main(int argc, char** argv)
{
	int pid = fork();
	if(pid == 0)
	{
		for(;;);
	} else if(pid > 0)
	{
		kill(pid, SIGKILL);
		int status;
		waitpid(pid, &status, 0x0);
		printf("Killing child...\t\t\t\t\t\t");
		if(status == 1)
			printf("[ OK ]\n");
		else printf("[FAIL]\n");
	}

	exit(0);
}
