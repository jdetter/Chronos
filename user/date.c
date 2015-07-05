#include "types.h"
#include "stdio.h"
#include "chronos.h"

int main(int argc, char** argv)
{
	struct timeval tv;

	if(gettimeofday(&tv, NULL))
	{
		printf("Error getting time from kernel.\n");
		exit(1);
	}

	printf("Unix time: %d\n", tv.tv_sec);

	exit(0);
}
