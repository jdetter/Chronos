#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARG 64

static const char* spawn_process = "/bin/sh";

int main(int argc, char** argv)
{
	printf("An init process has spawned on this tty.\n");
	fflush(stdout);
	int result = fork();
	if(result > 0)
	{
		waitpid(result, NULL, 0);
		printf("Warning: sh has exited.\n");
		for(;;);
	} else {
		const char* process = spawn_process;

		/* Check to see if were running the testsuite */
		int testsuite = open("/testsuite", O_RDONLY);
		if(testsuite >= 0)
		{
			close(testsuite);
			process = "/testsuite";
		}

		const char* args[MAX_ARG];
		memset(args, 0, MAX_ARG); 
		args[0] = process;
		args[1] = "arg1";
		args[2] = "args2";

		char* envp[1];
		envp[0] = NULL;
	
		printf("Spawning process %s...\n", args[0]);
		execve(process, (char* const*)args, (char* const*)envp);
		printf("init panic: exec has failed.\n");
		for(;;);
	}

	
	for(;;)puts("");
	return 0;
}
