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

tty_t tty_find(int index)
{
	if(index >= MAX_TTYS) return NULL;
	return ttys + index;
}

void tty_reset_sgr(tty_t t);
void tty_init(tty_t t, int num, char type, int cursor_enabled, 
		uintptr_t mem_start)
{
	memset(t, 0, sizeof(tty_t));

	t->num = num;
	t->active = 0; /* 1: This tty is in the foreground, 0: background*/
	t->type = type;
	t->tab_stop = 8;
	tty_reset_sgr(t); /* Reset the display properties */
	t->saved = 0;
	int i;
	for(i = 0; i<TTY_BUFFER_SZ; i+=2)/*sets text and graphics buffers*/
	{
		t->buffer[i]= ' ';
		t->buffer[i+1]= t->sgr.color;
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

int tty_num(tty_t t)
{
	if((char*)t >= (char*)ttys && (char*)t < (char*)(ttys + MAX_TTYS))
		return t->num;
	else panic("Invalid tty given to tty_num\n");
}

int tty_enable_code_logging(tty_t t)
{
	t->codes_logged = 1;
	return 0;
}

int tty_enable_logging(tty_t t, char* file)
{
	t->tty_log = klog_alloc(0, file);

	/* Did it get opened? */
	if(!t->tty_log)
		return -1;

	t->out_logged = 1;

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

tty_t tty_active(void)
{
	return active_tty;
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
	{
		if(t->out_logged && (c == '\n' || c == '\t'))
			klog_write(t->tty_log, &c, 1);

		return;
	}

	/* Is this tty logged? */
	if(t->out_logged)
		klog_write(t->tty_log, &c, 1);

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

tty_t tty_check(struct DeviceDriver* driver)
{
	uintptr_t addr = (uintptr_t)driver->io_driver.context;
	if(addr >= (uintptr_t)ttys && addr < (uintptr_t)ttys + MAX_TTYS)
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
