#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdio.h"

char* spawn_process = "/bin/sh";

void some_recursion(int i);

int main(int argc, char** argv)
{
	printf("An init process has spawned on this tty.\n");
	int result = fork();
	if(result > 0)
	{
		wait(result);
		printf("Warning: sh has exited.\n");
		for(;;);
	} else {
		char* args[MAX_ARG];
		memset(args, 0, MAX_ARG); 
		args[0] = spawn_process;
		args[1] = "arg1";
		args[2] = "args2";

		printf("Spawning process %s...\n", args[0]);
		exec(spawn_process, (const char**)args);
		printf("init panic: exec has failed.\n");
		for(;;);
	}

	
	for(;;)printf("");
	exit();
	return 0;
}

void some_recursion(int i)
{
	int result = i * 2;
	int x;
	for(x = 0;x < 100000;x++)
	{
		result = result / 2;
		result = result * 2;
	}
	printf("Recursion: 0x%x\n", result);
	if(i == 50) return;
	else some_recursion(i + 1);
}

