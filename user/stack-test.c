#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int recurse(int num, char* buff)
{
	char local[512];
	if(buff) memmove(local, buff, 512);
	if(num == 0) return 0;
	else return recurse(num - 1, local);
}

int main(int argc, char** argv)
{
	int pid = fork();
	if(pid > 0)
	{
		/* Parent */
		int result;
		waitpid(pid, &result, 0x0);

		printf("Testing stack...\t\t\t\t\t\t\t");
		if(result != 5)
			printf("[FAIL]\n");
		else printf("[ OK ]\n");
	} else if(pid == 0) {
		/* Child */
		recurse(10, NULL);
		exit(5);
	} else {
		printf("Fork failed!\n");
		exit(1);
	}

	pid = fork();
	if(pid > 0)
	{
		/* Parent */
		int result;
		waitpid(pid, &result, 0x0);

		printf("Massive stack test...\t\t\t\t\t\t\t");
		if(result != 5)
			printf("[FAIL]  %d\n", result);
		else printf("[ OK ]\n");
	} else if(pid == 0) {
		/* Child */
		recurse(2000, NULL);
		exit(5);
	} else {
		printf("Fork failed!\n");
		exit(1);
	}


	exit(0);
}
