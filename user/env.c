#include "chronos.h"
#include "stdio.h"

char* test_argv[] = 
{
	"/bin/exec-test",
	"These",
	"Are",
	"The",
	"Arguments"
};

char* test_env[] =
{
	"This",
	"Is",
	"An",
	"Environment",
	"Variable"
};

int main(int argc, char** argv, char** env)
{
	int f = fork();
	if(f == 0)
	{
		execve(test_argv[0], test_argv, test_env);
		printf("Execve failed!\n");
		exit(1);
	} else if(f > 0)
	{
		int stat;
		waitpid(f, &stat, 0x0);
		if(stat)
			printf("Child exit code: %d\n", stat);
	} else {
		printf("Fork failed!\n");
		exit(1);
	}

        exit(0);
        return 0;
}
