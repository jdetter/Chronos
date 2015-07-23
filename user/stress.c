#include "types.h"
#include "stdio.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdmem.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"

int main(int argc, char** argv)
{
	printf("Forking...\n");
	int x;
	for(x = 0;x < 100;x++)
	{
		int f = fork();
		if(!f)
		{
			printf("Execing child...\n");
			char* arg1 = "/bin/stall";
			const char* argv[] = {arg1, NULL};
			exec(arg1, argv);
		}

		int old = f;
		f = fork();
		if(!f)
		{
			printf("Execing child...\n");
                        char* arg1 = "/bin/stall";
                        const char* argv[] = {arg1, NULL};
                        exec(arg1, argv);
		} else { 
			printf("Parent wait.\n");
			waitpid(old, NULL, 0);
			waitpid(f, NULL, 0);
			printf("Parent is back.\n");
		}
	}

	exit(0);
}
