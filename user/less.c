#include "libtty.h"
#include "chronos.h"

int main(int argc, char** argv)
{
	/* Enter graphical mode */
	tty_mode_graphic();
	/* set the background to black */
	tty_set_forground(TTY_BLACK);
	/* Set the forground to gray */
	tty_set_background(TTY_LGRAY);
	/* Clear the screen */
	tty_clear_screen();
	/* Flush screen */
	tty_flush();
	for(;;);

	exit();
}
