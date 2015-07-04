#define __LINUX_DEFS__
#include "types.h"
//#include "proc.h"
#undef NULL

#include <stdio.h>
//#include "vm.h"

int main(int argc, char** argv)
{
	puts(".globl mktf");
	
	int x;
	for(x = 0;x < 255;x++)
	{
		printf(".globl handle_int_%d\n", x);
		printf("handle_int_%d:\n", x);
		if(x != 3)
			printf("\tpushl $0\n");
		printf("\tpushl $%d\n", x);
		printf("\tjmp mktf\n");
	}

	printf(".data\n");
	printf(".globl trap_handlers\n");
	printf("trap_handlers:\n");
	
	/* Create array */
	for(x = 0;x < 255;x++)
		printf("\t.long handle_int_%d\n", x);

	return 0;
}
