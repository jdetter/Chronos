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
#include "klog.h"

#define DEBUG 1

#ifdef DEBUG
#define cprintf ___cprintf
#endif

klog_t code_log;

int tty_code_log_init(void)
{
	code_log = klog_alloc(0, "console-codes.txt");
	if(!code_log)
		return -1;
	return 0;
}

/* Some useful hidden functions we need */
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
                console_putc(pos, c, t->sgr.color,
                                t->type==TTY_TYPE_COLOR,
                                (char*)t->mem_start);
        }

        /* Update back buffer */
        if(t->type == TTY_TYPE_COLOR)
        {
                char* vid_addr = t->buffer + (pos * 2);
                *(vid_addr)     = c;
                *(vid_addr + 1) = t->sgr.color;
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
                console_putc(t->cursor_pos, c, t->sgr.color,
                                t->type==TTY_TYPE_COLOR,
                                (char*)t->mem_start);
        }

        /* Update back buffer */
        if(t->type == TTY_TYPE_COLOR)
        {
                char* vid_addr = t->buffer
                        + (t->cursor_pos * 2);
                *(vid_addr)     = c;
                *(vid_addr + 1) = t->sgr.color;
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

	if(t->codes_logged)
	{
		/* Log this character */
		klog_write(code_log, &c, 1);
	}
}

void tty_set_cur_row(tty_t t, int row)
{
	if(row < 0) row = 0;
	if(row >= CONSOLE_ROWS) row = CONSOLE_ROWS - 1;

	int col = t->cursor_pos % CONSOLE_COLS;
	t->cursor_pos = (row * CONSOLE_COLS) + col;
	if(t->active) console_update_cursor(t->cursor_pos);
}

void tty_set_cur_col(tty_t t, int col)
{
	if(col < 0) col = 0;
	if(col >= CONSOLE_COLS) col = CONSOLE_COLS - 1;

	int row = t->cursor_pos / CONSOLE_COLS;
	t->cursor_pos = (row * CONSOLE_COLS) + col;
	if(t->active) console_update_cursor(t->cursor_pos);
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

/* Either erase to cursor or entire line */
void tty_erase_line(tty_t t, int entire_line)
{
	int row_start = t->cursor_pos - (t->cursor_pos % CONSOLE_COLS);
	if(entire_line)
		tty_erase_from_to_pos(t, row_start, row_start + CONSOLE_COLS);
	else tty_erase_from_to_pos(t, row_start, t->cursor_pos);
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

	if(t->codes_logged)
	{
		/* Log this nl */
		char nl = '\n';
		klog_write(code_log, &nl, 1);
	}
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

static void tty_save(tty_t t)
{
	t->save_cursor_pos = t->cursor_pos;
	t->save_cursor_enabled = t->cursor_enabled;
	memmove(&t->save_term, &t->term, sizeof(struct termios));
	memmove(&t->save_window, &t->window, sizeof(struct winsize));
	t->save_tab_stop = t->tab_stop;
	memmove(&t->save_sgr, &t->sgr, sizeof(struct sgr_attr));
	t->saved = 1;
}

static void tty_restore(tty_t t)
{
	if(!t->saved) return;
	t->cursor_pos = t->save_cursor_pos;
	t->cursor_enabled = t->save_cursor_enabled;
	memmove(&t->term, &t->save_term, sizeof(struct termios));
	memmove(&t->window, &t->save_window, sizeof(struct winsize));
	t->tab_stop = t->save_tab_stop;
	memmove(&t->sgr, &t->save_sgr, sizeof(struct sgr_attr));
}

static void tty_reset_foreground(tty_t t)
{
	t->sgr.color &= ~TTY_FORE_MASK;
	t->sgr.color |= TTY_FORE_GREY;
}

static void tty_reset_background(tty_t t)
{
	t->sgr.color &= ~TTY_BACK_MASK;
	t->sgr.color |= TTY_BACK_BLACK;
}

void tty_reset_sgr(tty_t t)
{
	memset(&t->sgr, 0, sizeof(struct sgr_attr));
	tty_reset_foreground(t);
	tty_reset_background(t);
}

static void tty_cursor_up(tty_t t, int rows)
{
	int curr_row = t->cursor_pos / CONSOLE_COLS;
	int curr_col = t->cursor_pos % CONSOLE_COLS;
	curr_row -= rows;
	if(curr_row <= 0)
		curr_row = 0;
	t->cursor_pos = (curr_row * CONSOLE_COLS) + curr_col;
	if(t->active) console_update_cursor(t->cursor_pos);
}

static void tty_cursor_down(tty_t t, int rows)
{
	int curr_row = t->cursor_pos / CONSOLE_COLS;
	int curr_col = t->cursor_pos % CONSOLE_COLS;
	curr_row += rows;
	if(curr_row >= CONSOLE_ROWS)
		curr_row = CONSOLE_ROWS - 1;
	t->cursor_pos = (curr_row * CONSOLE_COLS) + curr_col;
	if(t->active) console_update_cursor(t->cursor_pos);
}

static void tty_cursor_left(tty_t t, int cols)
{
	int curr_row = t->cursor_pos / CONSOLE_COLS;
	int curr_col = t->cursor_pos % CONSOLE_COLS;
	curr_col -= cols;
	if(curr_col <= 0)
		curr_col = 0;
	t->cursor_pos = (curr_row * CONSOLE_COLS) + curr_col;
	if(t->active) console_update_cursor(t->cursor_pos);
}

static void tty_cursor_right(tty_t t, int cols)
{
	int curr_row = t->cursor_pos / CONSOLE_COLS;
	int curr_col = t->cursor_pos % CONSOLE_COLS;
	curr_col += cols;
	if(curr_col >= CONSOLE_COLS)
		curr_col = CONSOLE_COLS - 1;
	t->cursor_pos = (curr_row * CONSOLE_COLS) + curr_col;
	if(t->active) console_update_cursor(t->cursor_pos);
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

static int tty_esc_type_sgr(tty_t t, char c, int pos, 
		int* params, int param_count)
{
	if(param_count == 0)
	{
		/* Reset the display attributes */
		tty_reset_sgr(t);
		return ESC_RES_DNE;
	}

	char err = 0;
	char incomplete = 0;
	int i;
	for(i = 0;i < param_count;i++)
	{
		switch(params[i])
		{
			case 0: /* Reset */
				tty_reset_sgr(t);
				break;
			case 1: /* Set bold */
				t->sgr.bold = 1;
				incomplete = 1;
				break;
			case 4: /* Set underscore */
				t->sgr.underscore = 1;
				incomplete = 1;
				break;
			case 5: /* Set background blink */
				t->sgr.color |= TTY_BACK_BLINK;
				break;
			case 7: /* Reverse the video output? */
				t->sgr.reverse_video = 1;
				incomplete = 1;
				break;
			case 24: /* Unset underscore */
				t->sgr.underscore = 0;
				incomplete = 1;
				break;
			case 25: /* Unset blink */
				t->sgr.color &= ~TTY_BACK_BLINK;
				break;
			case 27:
				t->sgr.reverse_video = 0;
				incomplete = 1;
				break;
			case 30: /* Black foreground */
				t->sgr.color &= ~TTY_FORE_COLOR_MASK;
				t->sgr.color |= TTY_FORE_BLACK;
				break;
			case 31: /* Red foreground */
				t->sgr.color &= ~TTY_FORE_COLOR_MASK;
				t->sgr.color |= TTY_FORE_RED;
				break;
			case 32: /* Green foreground */
				t->sgr.color &= ~TTY_FORE_COLOR_MASK;
				t->sgr.color |= TTY_FORE_GREEN;
				break;
			case 33: /* Brown foreground */
				t->sgr.color &= ~TTY_FORE_COLOR_MASK;
				t->sgr.color |= TTY_FORE_BROWN;
				break;
			case 34: /* Blue foreground */
				t->sgr.color &= ~TTY_FORE_COLOR_MASK;
				t->sgr.color |= TTY_FORE_BLUE;
				break;
			case 35: /* Magenta foreground */
				t->sgr.color &= ~TTY_FORE_COLOR_MASK;
				t->sgr.color |= TTY_FORE_MAGENTA;
				break;
			case 36: /* Cyan foreground */
				t->sgr.color &= ~TTY_FORE_COLOR_MASK;
				t->sgr.color |= TTY_FORE_CYAN;
				break;
			case 37: /* White foreground */
				t->sgr.color &= ~TTY_FORE_COLOR_MASK;
				t->sgr.color |= TTY_FORE_GREY;
				break;
			case 38:
				t->sgr.underscore = 1;
				/* Default foreground */
				tty_reset_foreground(t);
				incomplete = 1;
				break;
			case 39:
				t->sgr.underscore = 0;
				/* Default foreground */
				tty_reset_foreground(t);
				incomplete = 1;
				break;
			case 40: /* Black background */
				break;
				t->sgr.color &= ~TTY_BACK_COLOR_MASK;
				t->sgr.color |= TTY_BACK_BLACK;
			case 41: /* Red background */
				t->sgr.color &= ~TTY_BACK_COLOR_MASK;
				t->sgr.color |= TTY_BACK_RED;
				break;
			case 42: /* Green background */
				t->sgr.color &= ~TTY_BACK_COLOR_MASK;
				t->sgr.color |= TTY_BACK_GREEN;
				break;
			case 43: /* Brown background */
				t->sgr.color &= ~TTY_BACK_COLOR_MASK;
				t->sgr.color |= TTY_BACK_BROWN;
				break;
			case 44: /* Blue background */
				t->sgr.color &= ~TTY_BACK_COLOR_MASK;
				t->sgr.color |= TTY_BACK_BLUE;
				break;
			case 45: /* Magenta background */
				t->sgr.color &= ~TTY_BACK_COLOR_MASK;
				t->sgr.color |= TTY_BACK_MAGENTA;
				break;
			case 46: /* Cyan background */
				t->sgr.color &= ~TTY_BACK_COLOR_MASK;
				t->sgr.color |= TTY_BACK_CYAN;
				break;
			case 47: /* White background */
				t->sgr.color &= ~TTY_BACK_COLOR_MASK;
				t->sgr.color |= TTY_BACK_GREY;
				break;
			case 49: /* Default background */
				tty_reset_background(t);
				break;

				/* UNIMPLEMENTED BELOW */
			case 2: /* Half-bright */
			case 10: /* reset selected mapping */
			case 11: /* select null mapping */
			case 12:
			case 21: /* Set intensity */
			case 22:
				cprintf("UNIMPLEMENTED SGR: %d", params[i]);
				incomplete = 1;
				break;

			default: /* Invalid below */
				cprintf("INVALID SGR: %d", params[i]);
				err = 1;
				break;
		}
	}

	if(incomplete)
		cprintf("Warning: some SGR incomplete!");

	if(err)
	{
		cprintf("SOME INVALID SGR WAS ENCOUNTERED");
		return ESC_RES_ERR;
	}

	return ESC_RES_DNE; /* Done */
}

static int tty_esc_type_dec(tty_t t, char c, int pos)
{
	if(pos >= 7 && (c != 'h' && c != 'l'))
		return ESC_RES_ERR;

	/* Is c a numerical digit? */
	if(c >= '0' && c <= '9')
		return ESC_RES_CNT;

	/* Is this set or reset? */
	int set = 0;
	switch(c)
	{
		case 'l':
			set = 0;
			break;
		case 'h':
			set = 1;
			break;
		default:
			cprintf("INVALID DEC.");
			break;
	}

	if(set); /* FIXME: supress warning */

	int val = atoi(t->escape_chars + 2);
	cprintf("DEC VAL: %d\n", val);

	switch(val)
	{
		/* Unimplemented below here */
		case 1: /* DECCKM:  Escape modifier for arrow keys */
		case 3: /* DECCOLM: Column resolution switch */
		case 5: /* DECSCNM: Reverse video mode */
		case 6: /* DECOM:   Addresssing relative to scroll */
		case 7: /* DECAWM:  Autowrap */
		case 8: /* DECARM:  Keyboard autorepeat */
		case 9: /* X10 Mouse Reporting */
		case 25: /* Make cursor visible */
		case 1000: /* X11 Mouse Reporting */
			cprintf("DEC UNIMPLEMENTED");
			break;
	}

	return ESC_RES_ERR;
}

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
		params[param_count] = atoi(param_ptr);
		cprintf("%d parameter: %d", param_count, params[param_count]);
		/* Iterate to the first non num char */
		while(*param_ptr >= 0 && *param_ptr <= '9')
			param_ptr++;
		if(*param_ptr == ';') param_ptr++;
		param_count++;
	}

	cprintf("%d CSI parameters parsed.", param_count);

	switch(c)
	{
		case 'A':
			tty_cursor_up(t, params[0]);
			return ESC_RES_DNE;
		case 'e':
		case 'B':
			tty_cursor_down(t, params[0]);
			return ESC_RES_DNE;
		case 'C':
			tty_cursor_right(t, params[0]);
			return ESC_RES_DNE;
		case 'D':
			tty_cursor_left(t, params[0]);
			return ESC_RES_DNE;
		case 'E':
			tty_cursor_down(t, params[0]);
			tty_carrige_return(t);
			return ESC_RES_DNE;
		case 'F':
			tty_cursor_up(t, params[0]);
			tty_carrige_return(t);
			return ESC_RES_DNE;
		case 'G':
			tty_set_cur_col(t, params[0]);
			return ESC_RES_DNE;
		case 'f':
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
		case 'K':
			switch(params[0])
			{
				case 1:
					tty_erase_line(t, 0);
					break;
				case 2:
					tty_erase_line(t, 1);
					break;
				default:
					cprintf("INVALID CURSOR POS");
					break;
			}
			break;
		case 'm': /* SGR sequence */
			return tty_esc_type_sgr(t, c, pos,
					params, param_count);

			/* TODO: Ignored for now */
		case 'r': /* Change scrolling region */
			cprintf("WARNING: CSI Ignored!");
			break;

		case '?': /* DEC PRIVATE MODE? */
			if(param_count == 0)
			{
				t->escape_type = ESC_TYPE_DEC;
				return ESC_RES_CNT;
			}
			return ESC_RES_ERR;

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
		case '7': /* Save tty state */
			tty_save(t);
			return ESC_RES_DNE;
		case '8': /* restore tty state */
			tty_restore(t);
			return ESC_RES_DNE;
			/* Unimplemented escape sequences */
		case '#': /* Possible alignment test */

		case 'c': /* Reset the terminal properties */


		case 'M': /* Reverse line feed */

		case 'Z': /* DECID */

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
			case ESC_TYPE_DEC:
				handled = tty_esc_type_dec(t, c,
						t->escape_count - 1);
				break;
			default:
				handled = ESC_RES_ERR;
				cprintf("UNKNOWN SEQUENCE TYPE");
				break;
		}

		cprintf("ESC CHAR: handled? %d 0x%x %d %c", handled, c, c, c);

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
		case 0x9B: /* EQU TO ESC [ */
			strncpy(key_debug, "ESC START", 32);
			t->escape_seq = 1;
			t->escape_count = 0;
			t->escape_type = ESC_TYPE_ESC;
			memset(t->escape_chars, 0, NPAR);
			break;

			/* Unimplemented below here */
		case 0x18: /* int escape sequence start */
		case 0x1A:

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
	if(was_code) 
	{
		cprintf("Key: %s 0x%x %d %c", key_debug,
				c, c, c);
	}
#endif

	return was_code;
}
