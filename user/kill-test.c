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
		int y;
		while(1) 
		{
			int x;
			for(x = 0;x < 10000000;x++)
			{
				x++;
				x--;
			}
			y++;
		}
	} else if(pid > 0)
	{
		sleep(1);
		/* parent */
		printf("sending stop signal %d....\n", SIGSTOP);
		fflush(stdout);
		kill(pid, SIGSTOP);
		printf("Signal sent.\n");
		fflush(stdout);
		sleep(2);
		printf("sending cont signal...\n");
		fflush(stdout);
		kill(pid, SIGCONT);
		printf("signal sent.\n");
		fflush(stdout);

		sleep(2);
		kill(pid, SIGKILL);
	} else {
		printf("Fork failed!\n");
		return -1;
	}

	printf("Test was successful!");

	return 0;
}
