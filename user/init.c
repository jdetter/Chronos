#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdio.h"

char* spawn_process = "/bin/init";

int main(int argc, char** argv)
{
	printf("process init has spawned.");

	for(;;);
	int result = fork();
	if(result < 0)
	{
		wait(result);
		printf("Process completed\n");
	} else {
		char* args[] = {"", "arg1", "arg2"};
		args[0] = spawn_process;

		exec(spawn_process, (const char**)args);
	}

	
	for(;;);
	exit();
	return 0;
}
