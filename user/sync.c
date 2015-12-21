#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	sync();
	printf("Sync complete.\n");
	fflush(stdout);
	return 0;
}
