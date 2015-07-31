#include <stdio.h>

int main(int argc, char** argv, char** env)
{
	printf("Argument array:\n");
	int x;
	for(x = 0;x < argc;x++)
		printf("    %d: %s\n", x, argv[x]);
	printf("Environment:\n");
	for(x = 0;env[x];x++)
		printf("    %d: %s\n", x, env[x]);
		
	return 0;
}
