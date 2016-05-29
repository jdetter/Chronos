#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	int pid = fork();

	if(pid == 0)
	{
		/* Child */
		printf("Child spawned!\n");
		printf("Waiting for signal...\n");
		fflush(stdout);
		sigsuspend(0);
		printf("Got the signal!\n");
		printf("Test success!\n");
		fflush(stdout);
		return 0;
	} else if(pid > 0)
	{
		sleep(1);
		/* parent */
		kill(pid, SIGSTOP);
		printf("Sent signal\n");
		fflush(stdout);
		return 0;
	} else {
		printf("Fork failed!\n");
		return -1;
	}


	return 0;
}
