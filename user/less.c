#include "libtty.h"
#include "chronos.h"
#include "stdio.h"
#include "stdmem.h"
#include "stdlib.h"

void usage(void);

int main(int argc, char** argv)
{
	if(argc > 2)
	{
		usage();
		exit();
	}
	/* Enter graphical mode */
	tty_mode_graphic();
	/* set the background to black */
	tty_set_forground(TTY_LGRAY);
	/* Set the forground to gray */
	tty_set_background(TTY_BLACK);
	/* Clear the screen */
	tty_clear_screen();
	/* Flush screen */
	tty_flush();
	exit();
}


void usage(void)
{
	printf("Usage: less [file]\n");
}
