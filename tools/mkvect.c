#include "types.h"
#undef NULL

#include <stdio.h>
#include "vm.h"

char* interrupt_func = "interrupt_handler";
int interrupt_start = 0;
int trap_start = 32;
int max_interrupt = 255;
char* trap_func = "trap_handler";

int main(int argc, char** argv)
{
	puts("#include \"types.h\"");
	puts("#include \"idt.h\"");
	printf("\nvoid %s(void){}\n", interrupt_func);
	printf("void %s(void){}\n", trap_func);
	puts("\nstruct int_gate idt[] = {");

	int i;
	for(i = 0;i < max_interrupt;i++)
	{
		if(i < trap_start)
		{
			printf("\t{((uint_16)%s), (ss), "
				"((GATE_KERNEL) | GATE_INT_CONST), "
        			"((uint_16)(((uint)%s) >> 8))}", 
				interrupt_func, interrupt_func);
		} else {
			printf("\t{((uint_16)%s), (ss), "
                                "((GATE_KERNEL) | GATE_TRAP_CONST), "
                                "((uint_16)(((uint)%s) >> 8))}", 
				trap_func, trap_func);
		}

		if(i + 1 != max_interrupt) printf(",");
		printf("\n");
	}

	printf("};\n");	
	printf("\nuint size = sizeof(struct int_gate) * %d;\n\n", i);

	return 0;
}
