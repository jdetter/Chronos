#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	printf("Malloc...\t\t\t\t\t\t\t");
	void* ptr = malloc(0);
	if(ptr)
	{
		printf("[ OK ]\n");
	} else {
		printf("[FAIL]\n");
	}

	return 0;
}
