#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int sig)
{
	printf("Received signal: %d\n", sig);
}

int main(int argc, char** argv)
{
	struct sigaction action;
	action.sa_handler = handler;
	action.sa_mask = 0;
	action.sa_flags = 0;

	if(sigaction(SIGINT, &action, NULL))
	{
		printf("Couldn't assign handler!!\n");
		return -1;
	}

	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
	{
		printf("Could not ignore signal!\n");
		return -1;
	}

	if(kill(getpid(), SIGINT))
	{
		printf("Couldn't call kill!\n");
		return -1;
	}

	fflush(stdout);
	printf("Test successful!\n");

	return 0;
}
