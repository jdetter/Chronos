#include "cpu.h"

static int cli_count = 0;
void push_cli(void)
{
	if(cli_count == 0 && )
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
