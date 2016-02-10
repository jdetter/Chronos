#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>

static int call_rec(int i)
{
	int x;
	for(x = 0;x < 10000;x++);

	if(i != 0)
		return call_rec(i - 1);

	return i;
}

int main(int argc, char** argv)
{
	int x;
	for(x = 0;x < 64;x++)
	{
		char buff1[100];
		memset(buff1, 0, 100);

#define PROCS 16
		char procs[PROCS];

		int is_child = 0;
		int y = 0;
		for(y = 0;y < PROCS;y++)
		{
			int pid = fork();
			if(pid == 0)
			{
				is_child = 1;
				break;
			} else if(pid > 0)
			{
				procs[y] = pid;
			} else {
				printf("FORK FAILED\n");
				return -1;
			}
		}

		if(is_child)
		{
			call_rec(32);
			return 0;
		} 

		for(y = 0;y < PROCS;y++)
			waitpid(procs[y], NULL, 0);

		for(y = 0;y < 100;y++)
			if(buff1[y] != 0)
			{
				printf("TEST FAILED!!\n");
				return -1;
			}
	}

	printf("Context switch test passed.\n");
	return 0;
}
