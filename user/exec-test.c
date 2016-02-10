#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	char* args[3];
	args[0] = "/bin/echo";
	args[1] = "exec worked.";
	args[2] = NULL;

	printf("Running execvp...\n");
	execvp(args[0], args);
	printf("FAILED");
		
	return 0;
}
