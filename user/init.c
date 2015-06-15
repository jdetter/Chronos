#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdio.h"

char* spawn_process = "/bin/example";

int main(int argc, char** argv)
{
	printf("process init has spawned.\n");

	int result = fork();
	if(result > 0)
	{
		wait(result);
		printf("Process completed\n");
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

	
	for(;;);
	exit();
	return 0;
}
