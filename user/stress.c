#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

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
			char* const argv[] = {arg1, NULL};
			execve(arg1, argv, NULL);
		}

		int old = f;
		f = fork();
		if(!f)
		{
			printf("Execing child...\n");
                        char* arg1 = "/bin/stall";
                        char* const argv[] = {arg1, NULL};
                        execve(arg1, argv, NULL);
		} else { 
			printf("Parent wait.\n");
			waitpid(old, NULL, 0);
			waitpid(f, NULL, 0);
			printf("Parent is back.\n");
		}
	}

	return 0;
}
