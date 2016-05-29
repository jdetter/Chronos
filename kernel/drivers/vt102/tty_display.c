#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "panic.h"
#include "iosched.h"
#include "stdarg.h"
#include "stdlib.h"
#include "proc.h"
#include "fcntl.h"
#include "tty.h"
#include "drivers/serial.h"
#include "drivers/console.h"

void tty_putc_native(char c, tty_t t);
void tty_putc(tty_t t, char c)
{
	/* Write out to the serial connection */
	if(t->type==TTY_TYPE_SERIAL)
	{
		if(t->active) serial_write(&c,1);
		return;
	}

	/* Is this tty logged? */
	if(t->out_logged)
		klog_write(t->tty_log, &c, 1);

	/* Process console codes */
	if(tty_parse_code(t, c))
		return;

	if(t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_COLOR)
	{
		/* If this tty is active, output the char */
		if(t->active)
		{
			console_putc(t->cursor_pos, c, t->sgr.color, 
					t->type==TTY_TYPE_COLOR, 
					(char*)t->mem_start);
		}

		/* Update the back buffer */
		if(t->type==TTY_TYPE_COLOR)
		{
			char* vid_addr = t->buffer
				+ (t->cursor_pos * 2);
			*(vid_addr)     = c;
			*(vid_addr + 1) = t->sgr.color;
		}
		else
		{
			char* vid_addr = t->buffer
				+ (t->cursor_pos);
			*(vid_addr)     = c;
		}
		t->cursor_pos++;

		/* Scroll the screen if needed */
		if(t->cursor_pos >= CONSOLE_COLS * CONSOLE_ROWS)
		{
			tty_scroll(t);
			t->cursor_pos = (CONSOLE_COLS * CONSOLE_ROWS)
				- CONSOLE_COLS;
		}

		/* If this tty is active, update the on screen cursor */
		if(t->active)
			console_update_cursor(t->cursor_pos);
	}
	else
	{
		panic("TTY invalid graphics mode");
	}

}

void tty_scroll(tty_t t)
{
	if(!t->type==TTY_TYPE_MONO && !t->type==TTY_TYPE_COLOR)
		return;
	/* Bytes per row */
	int bpr = CONSOLE_COLS * 2;
	/* Rows on the screen */
	int rows = CONSOLE_ROWS;
	/* Back buffer */
	char* buffer = t->buffer;

	int x;
	for(x = 0;x < rows - 1;x++)
	{
		char* dst_row = buffer + x * bpr;
		char* src_row = dst_row + bpr;
		memmove(dst_row, src_row, bpr);
	}
	/* Unset the last line */
	memset(buffer + x * bpr, 0, bpr);

	/* If this tty is active, print this immediatly. */
	if(t->active)
		tty_print_screen(t, t->buffer);
}

void tty_print_screen(tty_t t, char* buffer)
{
	if(!t->active) return;

	if(t->type==TTY_TYPE_SERIAL)
	{
		return;
	} else if(t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_COLOR)
	{
		char* vid_addr = (char*)t->mem_start;
		uint sz = 0;
		if(t->type==TTY_TYPE_COLOR)
		{
			sz = CONSOLE_ROWS * CONSOLE_COLS * 2;
		} else {
			sz = CONSOLE_ROWS * CONSOLE_COLS;
		}
		memmove(vid_addr, buffer, sz);
	} else {
		panic("TTY invalid graphics mode");
	}
}

char tty_set_cursor(tty_t t, char enabled)
{
	t->cursor_enabled = enabled;
	return 0;
}

char tty_set_cursor_pos(tty_t t, int pos)
{
	t->cursor_pos = pos;
	return 0;
}
