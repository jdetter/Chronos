#include <stdlib.h>
#include <string.h>

#include "kern/types.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "panic.h"
#include "console.h"
#include "keyboard.h"
#include "tty.h"
#include "iosched.h"
#include "tty.h"
#include "proc.h"
#include "serial.h"

// #define KEY_DEBUG

extern struct tty* active_tty;

void tty_delete_char(tty_t t)
{
#ifdef KEY_DEBUG
	cprintf("kbd: deleting character...\n");
#endif
	char serial_back[] = {0x08, ' ', 0x08};
	switch(t->type)
	{
		default: 
		case TTY_TYPE_NONE: 
			break;
		case TTY_TYPE_COLOR: 
		case TTY_TYPE_MONO:
			if(t->cursor_pos == 0) break;
			t->cursor_pos--;
			tty_putc(t, ' ');
			t->cursor_pos--;
			/* update the cursor position */
			if(t->active) 
				console_update_cursor(t->cursor_pos);
			break;
		case TTY_TYPE_SERIAL:
			/* Send backspace, space, backspace */
			serial_write(serial_back, 3);
			break;
	}
}	

void tty_clear_input(tty_t t)
{
#ifdef KEY_DEBUG
	cprintf("kbd: Input line cleared.\n");
#endif
	slock_acquire(&t->key_lock);
	memset(&t->kbd_line, 0, sizeof(struct kbd_buff));
	memset(&t->keyboard, 0, sizeof(struct kbd_buff));
	slock_release(&t->key_lock);
}

extern slock_t ptable_lock;
void tty_handle_keyboard_interrupt(void)
{
	if(!active_tty) 
	{
		cprintf("tty: there is no active tty.\n");
		return;
	}

	slock_acquire(&ptable_lock);
	slock_acquire(&active_tty->key_lock);
	char c = 0;
	do
	{
		switch(active_tty->type)
		{
			default:
				cprintf("tty: active tty is invalid.\n");
				break;
			case TTY_TYPE_COLOR:
			case TTY_TYPE_MONO:
				c = kbd_getc();
				break;
			case TTY_TYPE_SERIAL:
				c = serial_read_noblock();
				break;
		}
		if(!c) break; /* no more characters */
		/* Got character. */

#ifdef KEY_DEBUG
		cprintf("Got character: %c\n", c);
#endif
		/* Preprocess the input (canonical mode only!) */
		uchar canon = active_tty->term.c_lflag & ICANON;
		//cprintf("Canonical mode: %d\n", canon);
		if(canon)
		{
			if(c == 13)
			{
#ifdef KEY_DEBUG
				cprintf("canon: \\r swapped for \\n");
#endif
				/* Carrige return */
				if(active_tty->term.c_iflag & IGNCR) continue;
				else if(active_tty->term.c_iflag & ICRNL) c = '\n';
			} else if(c == '\n')
			{
				if(active_tty->term.c_iflag & INLCR) 
					c = 13; /* Carriage return */

			}

			/* Check for case conversions */
			if(active_tty->term.c_iflag & IUCLC)
			{
				if(c >= 'A' && c <= 'Z')
					c += ('a' - 'A');
			}
		}

		/* Are we running in canonical mode? */
		if(canon)
		{
			/* Check for delete character */
			if(c == 0x7F || c == 0x08)
			{
#ifdef KEY_DEBUG
				cprintf("kbd: received delete char.\n");
#endif
				if(!tty_keyboard_delete(active_tty))
					tty_delete_char(active_tty);
				continue;
			} else {
				/* Write to the current line */
				tty_keyboard_write_buff(&active_tty->kbd_line, c);
				if(active_tty->kbd_line.key_nls)
				{
					/* A line delimiter was written */
#ifdef KEY_DEBUG
					cprintf("kbd: process signaled.\n");
#endif
					signal_keyboard_io(active_tty);
				}
			}

			/* Check for echo */
			if(active_tty->term.c_lflag & ECHO)
				tty_putc(active_tty, c);
		} else {
			if(tty_keyboard_write_buff(&active_tty->keyboard, c))
			{
				cprintf("Warning: keyboard buffer is full.\n");
			}
			/* signal anyone waiting for anything */
			signal_keyboard_io(active_tty);

		}

#ifdef KEY_DEBUG
		cprintf("kbd: NLS in line buffer: %d\n", 
			active_tty->kbd_line.key_nls);
		cprintf("kbd: Line buffer: %s\n", active_tty->kbd_line.buffer +
				active_tty->kbd_line.key_read);
#endif
	} while(1);


	slock_release(&ptable_lock);
	slock_release(&active_tty->key_lock);
}

static int tty_shandle(int pressed, int special, int val, int ctrl, int alt,
                int shift, int caps)
{
	cprintf("tty: special key received:\n");
	cprintf("    + pressed:   %d\n", pressed);
	cprintf("    + special:   %d\n", special);
	cprintf("    + val:       %d\n", val);
	cprintf("    + ctrl:      %d\n", ctrl);
	cprintf("    + alt:       %d\n", alt);
	cprintf("    + shift:     %d\n", shift);
	cprintf("    + caps:      %d\n", caps);

	if(!special)
		cprintf("    + character: %c\n", (char)val);

	return 0;
}

void tty_setup_kbd_events(void)
{
	kbd_event_handler(tty_shandle);
}

char tty_keyboard_write_buff(struct kbd_buff* kbd, char c)
{
	/* Check for full buffer */
	if(kbd->key_full) return -1;

	/* Wrap write position if needed */
	if(kbd->key_write >= TTY_KEYBUFFER_SZ)
		kbd->key_write = 0;

	/* Write the character */
	kbd->buffer[kbd->key_write] = c;
	/* Increment write position */
	kbd->key_write++;
	/* Wrap write position if needed */
	if(kbd->key_write >= TTY_KEYBUFFER_SZ)
		kbd->key_write = 0;
	/* Check for full */
	if(kbd->key_write == kbd->key_read)
		kbd->key_full = 1;	

	/* Did we just write a line delimiter? */
	if(c == '\n' || c == 13) kbd->key_nls++;
	return 0;
}

char tty_keyboard_read_buff(struct kbd_buff* kbd)
{
	/* Check for empty */
	if(kbd->key_write == kbd->key_read && !kbd->key_full)
		return 0;

	/* Reset full */
	kbd->key_full = 0;
	/* Wrap read position if needed */
	if(kbd->key_read >= TTY_KEYBUFFER_SZ)
		kbd->key_read = 0;
	/* Read the character */
	char c = kbd->buffer[kbd->key_read];
	/* Clear the character (Security) */
	kbd->buffer[kbd->key_read] = 0;

	/* Increment read position */
	kbd->key_read++;
	/* Wrap read position if needed */
	if(kbd->key_read >= TTY_KEYBUFFER_SZ)
		kbd->key_read = 0;
	/* Did we just read a line delimiter? */
	if(c == '\n' || c == 13) kbd->key_nls--;

	/* Return the character */
	return c;	
}

char tty_keyboard_delete(tty_t t)
{
	struct kbd_buff* keyboard = &t->kbd_line;
	/* Check for empty */
	if(keyboard->key_write == keyboard->key_read && 
			!keyboard->key_full) 
	{
#ifdef KEY_DEBUG
		cprintf("kbd: Tried to delete but buff is empty.\n");
#endif
		return -1;
	}

	/* Delete the character at the last position */
	if(keyboard->key_write == 0)
		keyboard->key_write = TTY_KEYBUFFER_SZ - 1;
	else keyboard->key_write--;

	/* Sanity check: is this character deletable? */
	if(keyboard->buffer[keyboard->key_write] == '\n' ||
			keyboard->buffer[keyboard->key_write] == 13)
		panic("tty: tried to delete a line delimiter!\n");

	/* erase the character */
	keyboard->buffer[keyboard->key_write] = 0;
	return 0;
}

char tty_keyboard_kill(tty_t t)
{
	int x;
	while(tty_keyboard_read_buff(&t->kbd_line))x++;
	memset(&t->kbd_line, 0, sizeof(struct kbd_buff));

	return x;
}

void tty_keyboard_flush_line(tty_t t)
{
	/* Flush the line buffer to the regular buffer */
	char c;
	while((c = tty_keyboard_read_buff(&t->kbd_line)))
		tty_keyboard_write_buff(&t->keyboard, c);
}

char tty_keyboard_read(tty_t t)
{
	char c;
	/* try to read from normal buffer first */
	if((c = tty_keyboard_read_buff(&t->keyboard)))
	{	
		/* Read success */
		return c;
	} // else cprintf("Keyboard buffer empty.\n");

	/* If were in canonical mode we can also read from the line buffer */
	if(t->term.c_lflag & ICANON)
	{
		if((c = tty_keyboard_read_buff(&t->kbd_line)))
		{
			/* Read success */
			return c;
		} // else cprintf("Line buffer empty.\n");
	}

	// cprintf("Read failure.\n");
	/* read failure: buffer empty */
	return 0;
}

int tty_keyboard_count(tty_t t)
{
	/* Count the amount of characters in the input buffer. */

	int characters = 0;
	if(t->term.c_lflag & ICANON)
	{
		/* Count the line buffer */
		if(t->kbd_line.key_full)
			characters += TTY_KEYBUFFER_SZ;
		/* Check for empty */
		else if(t->kbd_line.key_write == t->kbd_line.key_read);
		else if(t->kbd_line.key_read < t->kbd_line.key_write)
			characters += t->kbd_line.key_write -
				t->kbd_line.key_read;
		/* Calculate wrap */
		else characters += t->kbd_line.key_write 
			+ (TTY_KEYBUFFER_SZ - t->kbd_line.key_read);
	}

	/* Calculate back buffer */
	if(t->keyboard.key_full)
		characters += TTY_KEYBUFFER_SZ;
	/* Check for empty */
	else if(t->keyboard.key_write == t->keyboard.key_read);
	else if(t->keyboard.key_read < t->keyboard.key_write)
		characters += t->keyboard.key_write -
			t->keyboard.key_read;
	/* Calculate wrap */
	else characters += t->keyboard.key_write  
		+ (TTY_KEYBUFFER_SZ - t->keyboard.key_read);

	return characters;
}

