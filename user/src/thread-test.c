#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

int main(int argc, char** argv)
{
	char* args[3];
	args[0] = "/bin/echo";
	args[1] = "Child done.";
	args[2] = NULL;

	char* env[1];
	env[0] = NULL;

	printf("About to fork...\n");
	int pid = vfork();

	if(pid == 0)
	{
		execve(args[0], args, env);
	} else {
		printf("Parent done.\n");
	}

	return 0;
}
