#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

extern char** environ;

int main(int argc, char** argv, char** env)
{
	char** e = environ;
        for(;*e;e++) printf("%s\n", *e);
	return 0;
}
