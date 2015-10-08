#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	printf("sleeping for 1 second...\n");
	fflush(stdout);
	sleep(1);
	printf("sleeping for 2 seconds...\n");
	fflush(stdout);
	sleep(2);
	printf("sleeping for 3 seconds...\n");
	fflush(stdout);
	sleep(3);
	printf("sleeping for 5 seconds...\n");
	fflush(stdout);
	sleep(5);
	printf("sleeping for 10 seconds...\n");
	fflush(stdout);
	sleep(10);

	return 0;
}
