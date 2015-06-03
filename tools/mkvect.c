#include "types.h"
#undef NULL

#include <stdio.h>
#include "vm.h"

int main(int argc, char** argv)
{
	puts(".globl trap_handler");
	
	int x;
	for(x = 0;x < 255;x++)
	{
		printf(".globl handle_int_%d\n", x);
		printf("handle_int_%d:\n", x);
		printf("\tpush %d\n", x);
		printf("\tcall trap_handler\n");
	}

	return 0;
}
