#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARG 64

char* spawn_process = "/bin/sh";

void some_recursion(int i);

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
		char* args[MAX_ARG];
		memset(args, 0, MAX_ARG); 
		args[0] = spawn_process;
		args[1] = "arg1";
		args[2] = "args2";

		char* envp[1];
		envp[0] = NULL;
	
		printf("Spawning process %s...\n", args[0]);
		execve(spawn_process, (char* const*)args, (char* const*)envp);
		printf("init panic: exec has failed.\n");
		for(;;);
	}

	
	for(;;)puts("");
	return 0;
}
