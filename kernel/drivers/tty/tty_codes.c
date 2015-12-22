#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/fcntl.h>

#include "file.h"
#include "stdlock.h"
#include "kern/types.h"
#include "devman.h"
#include "console.h"
#include "serial.h"
#include "tty.h"
#include "fsman.h"

#define DEBUG 1

#ifdef DEBUG
#define cprintf ___cprintf

void* code_log;
int code_off;
#endif

/* Some useful hidden functions we need */

void tty_code_log_init(void)
{
	code_log = fs_open("/var/log/code.txt", 
		O_CREAT | O_RDWR, 0600, 0, 0);
	code_off = 0;
}

/* Delete a column */
void tty_delete_char(tty_t t);

/* Put a character on the screen at the given position */
void tty_native_setc(tty_t t, int pos, char c)
{
        /* Is this a serial tty? */
        if(t->type==TTY_TYPE_SERIAL)
        {
                // if(t->active) serial_write(&c, 1);
                return;
        }

        /* If this tty is active, print out to the screen */
        if(t->active)
        {
                console_putc(pos, c, t->color,
                                t->type==TTY_TYPE_COLOR,
                                (uchar*)t->mem_start);
        }

        /* Update back buffer */
        if(t->type == TTY_TYPE_COLOR)
        {
                char* vid_addr = t->buffer + (pos * 2);
                *(vid_addr)     = c;
                *(vid_addr + 1) = t->color;
        } else {
                char* vid_addr = t->buffer + (pos);
                *(vid_addr)     = c;
        }
}

/* Simply print a character (no processing) */
void tty_putc_native(tty_t t, char c)
{
        /* Is this a serial tty? */
        if(t->type==TTY_TYPE_SERIAL)
        {
                if(t->active) serial_write(&c, 1);
                return;
        }

        /* If this tty is active, print out to the screen */
        if(t->active)
        {
                console_putc(t->cursor_pos, c, t->color,
                                t->type==TTY_TYPE_COLOR,
                                (uchar*)t->mem_start);
        }

        /* Update back buffer */
        if(t->type == TTY_TYPE_COLOR)
        {
                char* vid_addr = t->buffer
                        + (t->cursor_pos * 2);
                *(vid_addr)     = c;
                *(vid_addr + 1) = t->color;
        } else {
                char* vid_addr = t->buffer
                        + (t->cursor_pos);
                *(vid_addr)     = c;
        }

	/* Move the cursor forward */
        t->cursor_pos++;

        /* Update the on screen cursor position if needed */
        if(t->active)
                console_update_cursor(t->cursor_pos);
}

void tty_set_cur_rc(tty_t t, int row, int col)
{
	t->cursor_pos = (row * CONSOLE_COLS) + col;
	if(t->active) console_update_cursor(t->cursor_pos);
}

void tty_erase_from_to_pos(tty_t t, int start_pos, int end_pos)
{
	if(end_pos <= start_pos) return;
        if(end_pos > (CONSOLE_ROWS * CONSOLE_COLS) - 1)
                end_pos = (CONSOLE_ROWS * CONSOLE_COLS) - 1;

        int pos = start_pos;
        for(;pos < end_pos;pos++)
                tty_native_setc(t, pos, ' ');
}

void tty_erase_from_to(tty_t t, int srow, int scol, int erow, int ecol)
{
	int start_pos = (srow * CONSOLE_COLS) + scol;
	int end_pos = (erow * CONSOLE_COLS) + ecol;

	tty_erase_from_to_pos(t, start_pos, end_pos);
}

void tty_erase_display(tty_t t)
{
	tty_erase_from_to(t, 0, 0, CONSOLE_ROWS, CONSOLE_COLS);
}

/* Print out a string (Debug for tty only) */
void tty_puts_native(tty_t t, char* str)
{
	for(;*str;str++)
		tty_putc_native(t, *str);
}

/* do a carriage return (go to beginning of line) */
static void tty_carrige_return(tty_t t)
{
	t->cursor_pos -= t->cursor_pos % CONSOLE_COLS;
	if(t->active) console_update_cursor(t->cursor_pos);
}

/* Do a line feed */
static void tty_new_line(tty_t t)
{
	t->cursor_pos += CONSOLE_COLS - 1;
        t->cursor_pos -= t->cursor_pos % CONSOLE_COLS;

	/* Scroll when needed */
	if(t->cursor_pos >= CONSOLE_COLS * CONSOLE_ROWS)
	{
		tty_scroll(t);
		t->cursor_pos = (CONSOLE_COLS * CONSOLE_ROWS)
                                - CONSOLE_COLS;
	}

	/* Adjust cursor position */
	tty_carrige_return(t);
}

/* Output a tab */
static void tty_tab(tty_t t)
{
	if(t->cursor_pos % t->window.ws_col == 0)
		return;
	int new_pos = t->cursor_pos + t->tab_stop;
	/* Round to the closest tab */
	new_pos -= new_pos % t->tab_stop;
	int x;
	for(x = 0;t->cursor_pos < new_pos;x++)
		tty_putc(t, ' ');
}

#ifdef DEBUG
static void ___cprintf(char* fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	char text[128];
	memset(text, 0, 128);
	vsnprintf(text, 128, fmt, list);
	va_end(list);

	tty_puts_native(tty_find(0), text);
	tty_new_line(tty_find(0));
}
#endif

/** Return results for parsing */
#define ESC_RES_ERR 0x00 /* Sequence is invalid */
#define ESC_RES_DNE 0x01 /* Sequence is done */
#define ESC_RES_CNT 0x02 /* Continue sequence */

static int tty_esc_type_csi(tty_t t, char c, int pos)
{
	if(c >= '0' && c <= '9') 
		return ESC_RES_CNT;
	if(c == ';')
		return ESC_RES_CNT;

	int params[8];
	int param_count = 0;
	memset(params, 0, sizeof(int) * 8);
	char* param_ptr = t->escape_chars + 1;
	while(*param_ptr >= 0 && *param_ptr <= '9')
	{
		cprintf("about to parse: %s", param_ptr);
		params[param_count] = atoi(param_ptr);
		cprintf("Got: %d", params[param_count]);
		/* Iterate to the first non num char */
		while(*param_ptr >= 0 && *param_ptr <= '9')
			param_ptr++;
		if(*param_ptr == ';') param_ptr++;
		param_count++;

		cprintf("%d parameter: %d", param_count, params[param_count]);
	}

	cprintf("%d CSI parameters parsed.", param_count);
	
	switch(c)
	{
		case 'H':
			tty_set_cur_rc(t, params[0], params[1]);
			return ESC_RES_DNE;
		case 'J':
			switch(params[0])
			{
				case 1:
					tty_erase_from_to_pos(t, 0, 
							t->cursor_pos);
					break;
				case 2:
				case 3:
					tty_erase_display(t);
					break;
				default:
					cprintf("INVALID CURSOR POS");
					break;
			}
			
			
			return ESC_RES_DNE;

		default:
			cprintf("INVALID CSI SEQUENCE");
			return ESC_RES_ERR;
	}
			
	return ESC_RES_ERR;
}

static int tty_esc_type_esc(tty_t t, char c, int pos)
{
	switch(c)
	{
		case '[': /* CSI START */
			t->escape_type = ESC_TYPE_CSI;
			cprintf("Started CSI sequence");
			return ESC_RES_CNT;
		case '(': /* Define G0 character set */
		case ')': /* Define G1 character set */
		case ']': /* OS command (change colors) */
			return ESC_RES_CNT;
		case 'D': /* Line feed */
		case 'E': /* New line */
			tty_new_line(t);
			return ESC_RES_DNE;

		case 'H': /* Set tab stop at curr pos */
			cprintf("Old tab stop: %d", t->tab_stop);
			t->tab_stop = t->cursor_pos % CONSOLE_COLS;
			cprintf("New tab stop: %d", t->tab_stop);
			return ESC_RES_DNE;
			/* Unimplemented escape sequences */
		case '#': /* Possible alignment test */

		case 'c': /* Reset the terminal properties */


		case 'M': /* Reverse line feed */

		case 'Z': /* DECID */

		case '7': /* Save tty state */

		case '8': /* restore tty state */

		case '>': /* Set numeric keyboard mode */

		case '=': /* Set application keyboard mode */

		default:
			/* Unhandled */
			return ESC_RES_ERR;
	}
}

int tty_parse_code(tty_t t, char c)
{

#ifdef DEBUG
	char key_debug[32];
	memset(key_debug, 0, 32);
#endif

	if(t->escape_seq)
	{
		/* Add this char to the sequence */
		t->escape_chars[t->escape_count] = c;
		t->escape_count++;

		int handled = 1;
		switch(t->escape_type)
		{
			case ESC_TYPE_ESC:
				handled = tty_esc_type_esc(t, c, 
						t->escape_count - 1);
				break;
			case ESC_TYPE_CSI:
				handled = tty_esc_type_csi(t, c,
						t->escape_count - 1);
				break;
			default:
				handled = ESC_RES_ERR;
				cprintf("UNKNOWN SEQUENCE TYPE");
				break;
		}

		cprintf("ESC CHAR: handled? %d 0x%x %d %c", handled, c, c, c);
		
		if(code_log)
		{
			char log[128];
			snprintf(log, 128, 
				"ESC CHAR: handled? %d 0x%x %d %c\n", 
				handled, c, c, c);
			fs_write(code_log, log, strlen(log), code_off);
			code_off += strlen(log);
		}

		if(handled == ESC_RES_DNE)
		{
			t->escape_count = 0;
			t->escape_type = 0;
			t->escape_seq = 0;
		} else if(handled == ESC_RES_ERR)
		{
			t->escape_count = 0;
			t->escape_type = 0;
			t->escape_seq = 0;
			cprintf("ERROR PARSING.");
		}

		/* This char was part of an escape sequence */
		return 1;

	}

	int was_code = 1;
	switch((unsigned char)c)
	{

		case 0x0D: /* Carrige Return */
			strncpy(key_debug, "CR", 32);
			tty_carrige_return(t);
			break;

		case 0x0A: /* line feed */
		case 0x0B:
		case 0x0C:
			strncpy(key_debug, "New Line", 132);
			tty_new_line(t);
			break;

		case 0x08: /* backspace */
			strncpy(key_debug, "Backspace", 32);
			tty_delete_char(t);
			break;

		case 0x09: /* tab */
			strncpy(key_debug, "Tab", 32);
			tty_tab(t);
			break;
		case 0x1B: /* Escape sequence start */
			strncpy(key_debug, "ESC START", 32);
			t->escape_seq = 1;
			t->escape_count = 0;
			t->escape_type = ESC_TYPE_ESC;
			memset(t->escape_chars, 0, NPAR);
			break;

			/* Unimplemented below here */
		case 0x0E: /* use G1 character set */

		case 0x0F: /* use G0 character set */

		case 0x18: /* int escape sequence start */
		case 0x1A:

		case 0x9B: /* EQU TO ESC [ */
			snprintf(key_debug, 32, 
					"UNIMPLEMENTED: %d 0x%x", c, c);
			break;

			/* nop below here */
		case 0x7F: /* Delete (ignored) */
		case 0x07: /* beep */
		default:
			was_code = 0;
			break;
	}

#ifdef DEBUG
	if(was_code) cprintf("Key: %s", key_debug);
#endif

	return was_code;
}
