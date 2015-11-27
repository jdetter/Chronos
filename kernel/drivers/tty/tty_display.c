#include <stdlib.h>
#include <string.h>

#include "kern/types.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "tty.h"
#include "panic.h"
#include "iosched.h"
#include "serial.h"
#include "console.h"
#include "stdarg.h"
#include "stdlib.h"
#include "proc.h"
#include "fcntl.h"

static struct tty ttys[MAX_TTYS];
struct tty* active_tty = NULL;

tty_t tty_find(uint index)
{
	if(index >= MAX_TTYS) return NULL;
	return ttys + index;
}

void tty_init(tty_t t, uint num, uchar type, uint cursor_enabled, 
		uint mem_start)
{
	memset(t, 0, sizeof(tty_t));

	t->num = num;
	t->active = 0; /* 1: This tty is in the foreground, 0: background*/
	t->type = type;
	t->color = TTY_BACK_BLACK|TTY_FORE_GREY;
	int i;
	for(i = 0; i<TTY_BUFFER_SZ; i+=2)/*sets text and graphics buffers*/
	{
		t->buffer[i]= ' ';
		t->buffer[i+1]= t->color;
	}
	t->cursor_pos = 0; /* Text mode position of the cursor. */
	t->cursor_enabled= cursor_enabled; /* Whether or not to show the cursor (text)*/
	t->mem_start = mem_start; /* The start of video memory. (color, mono only)*/

	/* Init keyboard lock */
	slock_init(&t->key_lock);

	/* Set the window spec */
	t->window.ws_row = CONSOLE_ROWS;
	t->window.ws_col = CONSOLE_COLS;
	
	/* Set the behavior */
	t->term.c_iflag = IGNBRK | IGNPAR | ICRNL;
	t->term.c_oflag = OCRNL | ONLRET;
	t->term.c_cflag = 0;
	t->term.c_lflag = ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOCTL;

	/* TODO: handle control characters */
	memset(t->term.c_cc, 0, NCCS);
}

uint tty_num(tty_t t)
{
	return t->num;
}

int tty_enable_logging(tty_t t, char* file)
{
	void* ino = fs_open(file, O_CREAT | O_RDWR, 0600, 0, 0);
	if(!ino)
		return -1;

	t->out_logged = 1;
	t->out_inode = ino;
	
	return 0;
}

void tty_enable(tty_t t)
{
	/* unset the currently active tty */
	if(active_tty)
		active_tty->active = 0;

	t->active = 1;
	active_tty = t;
	if(t->type==TTY_TYPE_SERIAL)
	{
		return;
	}
	if(t->type == TTY_TYPE_MONO || t->type == TTY_TYPE_COLOR)
	{
		tty_print_screen(t, t->buffer);
		console_update_cursor(t->cursor_pos);
	}
}
void tty_disable(tty_t t)
{
	t->active=0;
}

void tty_putc_native(char c, tty_t t);
void tty_putc(tty_t t, char c)
{
	/* Write out to the serial connection */
        if(t->type==TTY_TYPE_SERIAL)
        {
                if(t->active) serial_write(&c,1);
                return;
        }

	/* Process console codes */
	if(tty_parse_code(t, c))
		return;

	/* Is this tty logged? */
	if(t->out_logged)
	{
		int res = fs_write(t->out_inode, &c, 1, 
			t->out_file_pos);
		if(res == 1) t->out_file_pos++;
	}

	if(t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_COLOR)
	{
		/* If this tty is active, output the char */
		if(t->active)
		{
			console_putc(t->cursor_pos, c, t->color, 
					t->type==TTY_TYPE_COLOR, 
					(uchar*)t->mem_start);
		}

		/* Update the back buffer */
		if(t->type==TTY_TYPE_COLOR)
		{
			char* vid_addr = t->buffer
				+ (t->cursor_pos * 2);
			*(vid_addr)     = c;
			*(vid_addr + 1) = t->color;
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

tty_t tty_check(struct DeviceDriver* driver)
{
	uint addr = (uint)driver->io_driver.context;
	if(addr >= (uint)ttys && addr < (uint)ttys + MAX_TTYS)
	{
		/**
		 * addr points into the table so this is most
		 * likely a tty.
		 */
		return (tty_t)addr;
	}

	return NULL;
}

void tty_scroll(tty_t t)
{
	if(!t->type==TTY_TYPE_MONO && !t->type==TTY_TYPE_COLOR)
		return;
	/* Bytes per row */
	uint bpr = CONSOLE_COLS * 2;
	/* Rows on the screen */
	uint rows = CONSOLE_ROWS;
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

uchar tty_set_cursor(tty_t t, uchar enabled)
{
	t->cursor_enabled = enabled;
	return 0;
}

uchar tty_set_cursor_pos(tty_t t, uint pos)
{
	t->cursor_pos = pos;
	return 0;
}

void tty_set_color(tty_t t, uchar color)
{
	t->color = color;
}
