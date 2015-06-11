#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdio.h"

int main(int argc, char** argv)
{
	int x;
	for(x = 0;;x++)
	{
		printf("Line 1\n");
		printf("Line 2\n");
		printf("Line 3\n");
		printf("Line 4\n");
	}
	printf("Number: %d\n", 12);
	printf("Hex: 0x%p\n", 0x12);
	printf("chars: 0x%c%c\n", '1', '2');

	int pid = fork();
	if(pid)
	{
		printf("parent");
	} else {
		printf("child");	
	}
	
	for(;;) write(1, NULL, 0);
	exit();
	return 0;
}
