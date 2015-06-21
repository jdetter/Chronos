#include "types.h"
#include "libtty.h"
#include "chronos.h"

static char tty_buffer[4000];
static uchar color;

void tty_mode_graphic(void)
{
	tty_mode(1);
}

void tty_mode_text(void)
{
	tty_mode(0);
}

void tty_print_cell(int row, int col, char c)
{
	if(row < 0 || row >= TTY_ROWS || col < 0 || col >= TTY_COLS)
		return;
	int pos = (TTY_COLS * row * 2) + (c * 2);
	tty_buffer[pos] = c;
	tty_buffer[pos + 1] = color;
}

void tty_set_cursor(int row, int col)
{
	int pos = (TTY_COLS * row) + col;
	tty_cursor(pos);
}

void tty_clear_screen(void)
{
	/* Use current color to fill the screen. */
	int x;
	for(x = 0;x < TTY_ROWS * TTY_COLS;x++)
	{
		int pos = x * 2;
		tty_buffer[pos] = ' ';
		tty_buffer[pos + 1] = color;
	}
}

void tty_set_forground(uchar new_color)
{
	if(new_color < TTY_BLACK || new_color > TTY_WHITE) return;
        color &= 0xF0; /* Clear bottom bits */
        color |= (new_color << 0);
}

void tty_set_background(uchar new_color)
{
	if(new_color < TTY_BLACK || new_color > TTY_WHITE) return;
	color &= 0x0F; /* Clear top bits */
	color |= (new_color << 4);
}

void tty_flush(void)
{
	tty_screen(tty_buffer);
}
