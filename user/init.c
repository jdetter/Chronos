#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdio.h"

int main(int argc, char** argv)
{
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
