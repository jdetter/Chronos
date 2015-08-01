#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	int pid = fork();
	if(pid > 0)
	{
		/* Parent */
		int result;
		waitpid(pid, &result, 0x0);

		printf("Testing return...\t\t\t\t\t\t\t");
		if(result != 5)
			printf("[FAIL]\n");
		else printf("[ OK ]\n");
	} else if(pid == 0) {
		/* Child */
		return 5;
	} else {
		printf("Fork failed!\n");
		return 1;
	}

	return 0;
}
