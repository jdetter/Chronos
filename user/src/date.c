#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>

int main(int argc, char** argv)
{
	struct timeval tv;

	if(gettimeofday(&tv, NULL))
	{
		printf("Error getting time from kernel.\n");
		return 1;
	}

	printf("Unix time: %d\n", (int)tv.tv_sec);

	return 0;
}
