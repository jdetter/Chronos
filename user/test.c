#include "types.h"
#include "stdio.h"
#include "stdmem.h"

int main(int argc, char** argv)
{
	mem_debug((void (*)(char*, ...))printf);
	int* nums[128];
	int x;
	for(x = 0;x < 128;x++)
	{
		nums[x] = malloc(sizeof(int));
		*nums[x] = x;
	}
	for(x = 0;x < 128;x++)
		if(mfree(nums[x]))
		{
			printf("Free failed.\n");
		}

	printf("Tester program finished.\n");

	return 0;
}
