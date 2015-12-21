#include <sys/types.h>
#include <string.h>

#include "file.h"
#include "stdlock.h"
#include "kern/types.h"
#include "devman.h"
#include "console.h"
#include "serial.h"
#include "tty.h"

#define DEBUG 1

#ifdef DEBUG
#define puts(param) tty_puts_native(param, t)
#endif

/* Some useful hidden functions we need */

/* Delete a column */
void tty_delete_char(tty_t t);

/* Simply print a character (no processing) */
void tty_putc_native(char c, tty_t t)
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

/* Print out a string (Debug for tty only) */
void tty_puts_native(char* str, tty_t t)
{
	for(;*str;str++)
		tty_putc_native(*str, t);
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
	int new_pos = t->cursor_pos + 8;
	/* Round to the closest tab */
	new_pos -= new_pos % 8;
	int x;
	for(x = 0;t->cursor_pos < new_pos;x++)
		tty_putc(t, ' ');
}

int tty_parse_code(tty_t t, char c)
{
	if(t->escape_seq)
	{
		if(!t->escape_count)
		{
			switch(c)
			{
				case '[': /* CSI START */
				case '#': /* Possible alignment test */
				case '(': /* Define G0 character set */
				case ')': /* Define G1 character set */
				case ']': /* OS command (change colors) */
					t->escape_count++;
					t->escape_chars[0] = c;
#ifdef DEBUG
					puts("tty: esc started.\n");	
#endif
					break;

					/* Unimplemented escape sequences */
				case 'c': /* Reset the terminal properties */

				case 'D': /* Line feed */

				case 'E': /* New line */

				case 'H': /* Set tab stop at curr pos */

				case 'M': /* Reverse line feed */

				case 'Z': /* DECID */

				case '7': /* Save tty state */

				case '8': /* restore tty state */

				case '>': /* Set numeric keyboard mode */

				case '=': /* Set application keyboard mode */

				default:
					t->escape_seq = 0;
#ifdef DEBUG
					puts("tty: unknown escape seq.\n");
#endif
					break;
			}
		} else {
			puts("tty: advanced sequence encountered.\n");
		}

		/* This char was part of an escape sequence */
		return 1;
	}

	int was_code = 1;
	switch((unsigned char)c)
	{

		case 0x0D: /* Carrige Return */
			tty_carrige_return(t);
			break;

		case 0x0A: /* line feed */
		case 0x0B:
		case 0x0C:
			tty_new_line(t);
			break;

		case 0x08: /* backspace */
			tty_delete_char(t);
			break;

		case 0x09: /* tab */
			tty_tab(t);
			break;
		case 0x1B: /* Escape sequence start */
			t->escape_seq = 1;
			t->escape_count = 0;
			memset(t->escape_chars, 0, NPAR);

#ifdef DEBUG
			puts("tty: Escape sequence started\n");
#endif
			break;

			/* Unimplemented below here */
		case 0x0E: /* use G1 character set */

		case 0x0F: /* use G0 character set */

		case 0x18: /* int escape sequence start */
		case 0x1A:

		case 0x9B: /* EQU TO ESC [ */
			break;

			/* nop below here */
		case 0x7F: /* Delete (ignored) */
		case 0x07: /* beep */
		default:
			was_code = 0;
			break;
	}

	return was_code;
}
