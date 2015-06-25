#include "types.h"
#include "cpu.h"
#include "panic.h"

uchar __check_interrupt__(void);

static int cli_count = 0;
void push_cli(void)
{
	/**
	 * If we pushcli and interrupts are disabled, a popcli could 
	 * enable interrupts. To prevent this, we are setting cli_count to 1.
	 */
	if(cli_count == 0 && !__check_interrupt__()) cli_count = 1;
        if(cli_count < 0) cli_count = 0;
        cli_count++;
        asm volatile("cli");
}

void pop_cli(void)
{
        cli_count--;
        if(cli_count < 1)
        {
                cli_count = 0;
                asm volatile("sti");
        }
}

void reset_cli(void)
{
	cli_count = 0;
}
