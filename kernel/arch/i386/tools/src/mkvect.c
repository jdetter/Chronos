#define __LINUX_DEFS__
#include <stdio.h>

int main(int argc, char** argv)
{
	puts(".globl tp_mktf");
	
	int x;
	for(x = 0;x < 256;x++)
	{
		printf(".globl handle_int_%d\n", x);
		printf("handle_int_%d:\n", x);
		switch(x)
		{
			/* These exceptions DO generate an exception code */
			case  8: /* Double fault */
			case 10: /* Invalid TSS */
			case 11: /* Segment not present */
			case 12: /* Stack segment fault */
			case 13: /* general protection */
			case 14: /* Page fault */
			case 17: /* Alignment check */

			/* For all NO GENERATE exceptions, push a 0 */
			break;
			default:
				printf("\tpushl $0\n");
				break;
		}
		printf("\tpushl $%d\n", x);
		printf("\tjmp tp_mktf\n");
	}

	printf(".data\n");
	printf(".globl trap_handlers\n");
	printf("trap_handlers:\n");
	
	/* Create array */
	for(x = 0;x < 256;x++)
		printf("\t.long handle_int_%d\n", x);

	return 0;
}
