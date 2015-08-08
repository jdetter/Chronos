#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "tty.h"
#include "panic.h"
#include "serial.h"
#include "console.h"
#include "stdarg.h"
#include "stdmem.h"
#include "stdlib.h"
#include "keyboard.h"
#include "iosched.h"
#include "ioctl.h"
#include "fsman.h"
#include "pipe.h"
#include "proc.h"

extern struct proc* rproc;
struct tty ttys[MAX_TTYS];
static struct tty* active = NULL;

int tty_io_init(struct IODriver* driver);
int tty_io_read(void* dst, uint start_read, uint sz, void* context);
int tty_io_write(void* src, uint start_write, uint sz, void* context);
int tty_io_ioctl(unsigned long request, void* arg, tty_t context);
int tty_io_setup(struct IODriver* driver, uint tty_num)
{
	/* Get the tty */
	tty_t t = tty_find(tty_num);
	driver->context = t;
	driver->init = tty_io_init;
	driver->read = tty_io_read;
	driver->write = tty_io_write;
	driver->ioctl = (int (*)(unsigned long, void*, void*))tty_io_ioctl;
	return 0;
}

int tty_io_init(struct IODriver* driver)
{
	return 0;
}

int tty_io_read(void* dst, uint start_read, uint sz, void* context)
{
	// tty_t t = context;
	// cprintf("Initiated read for max of %d characters.\n", sz);
	sz = tty_gets(dst, sz);
	// cprintf("Returning with %d bytes.\n", sz);
	// cprintf("Buffer returned: %s\n", (char*)dst);
	return sz;
	/*int x;
	for(x = 0;x < sz;x++, dst_c++)
	{
		*dst_c = tty_getc(t);
		if(*dst_c == '\n') break;
	}

	cprintf("Read finished with %d bytes.\n", x);
	return sz; */
}

int tty_io_write(void* src, uint start_write, uint sz, void* context)
{
	tty_t t = context;
	uchar* src_c = src;
	int x;
	for(x = 0;x < sz;x++, src_c++)
		tty_putc(t, *src_c);
	return sz;
}

int tty_io_ioctl(unsigned long request, void* arg, tty_t t)
{
	uint i = (uint)arg;
	int* ip = (int*)arg;
	struct winsize* windowp = arg;
	struct termios* termiosp = arg;
	struct termio*  termiop = arg;
	pid_t* pgidp = arg;
	pid_t* sidp = arg;

	switch(request)
	{
		case TCGETS:
			if(!ioctl_arg_ok(arg, sizeof(struct termios)))
				return -1;
			memmove(arg, &t->term, sizeof(struct termios));
			break;
		case TCSETSF:
			if(!ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			/* Clear the input buffer */
			tty_clear_input(t);
			break;
		case TCSETSW:
		case TCSETS:
			if(!ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			memmove(&t->term, arg, sizeof(struct termios));
			break;
                case TCGETA:
                        if(!ioctl_arg_ok(arg, sizeof(struct termio)))
				return -1;
			/* do settings */
			t->term.c_iflag = termiop->c_iflag;
			t->term.c_oflag = termiop->c_oflag;
			t->term.c_cflag = termiop->c_cflag;
			t->term.c_lflag = termiop->c_lflag;
			t->term.c_line = termiop->c_line;
			memmove(&t->term.c_cc, termiop->c_cc, NCCS);

			break;
		case TCSETAF:
			if(!ioctl_arg_ok(arg, sizeof(struct termio)))
				return -1;
			tty_clear_input(t);
			/* do settings */
			t->term.c_iflag = termiop->c_iflag;
			t->term.c_oflag = termiop->c_oflag;
			t->term.c_cflag = termiop->c_cflag;
			t->term.c_lflag = termiop->c_lflag;
			t->term.c_line = termiop->c_line;
			memmove(&t->term.c_cc, termiop->c_cc, NCCS);
			break;
		case TCSETAW:
		case TCSETA:
			if(!ioctl_arg_ok(arg, sizeof(struct termio)))
				return -1;
			/* do settings */
			t->term.c_iflag = termiop->c_iflag;
			t->term.c_oflag = termiop->c_oflag;
			t->term.c_cflag = termiop->c_cflag;
			t->term.c_lflag = termiop->c_lflag;
			t->term.c_line = termiop->c_line;
			memmove(&t->term.c_cc, termiop->c_cc, NCCS);
			break;
		case TIOCGLCKTRMIOS:
			if(!ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			if(t->termios_locked)
				memset(termiosp, 0xFF,sizeof(struct termios));
			else memset(termiosp, 0x00, sizeof(struct termios));
			break;
		case TIOCSLCKTRMIOS:
			if(!ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			if(termiosp->c_iflag) /* new value is locked */
				t->termios_locked = 1;
			else t->termios_locked = 0; /* new value is unlocked*/	
			break;
		case TIOCGWINSZ:
			if(ioctl_arg_ok(arg, sizeof(struct winsize)))
				return -1;
			memmove(windowp, &t->window, sizeof(struct winsize));
			break;
		case TIOCSWINSZ: /* Not allowed right now. */
			cprintf("Warning: %d tried to change the"
					" window size.\n", rproc->name);
			return -1;
			break;

			// case TIOCINQ:
		case FIONREAD:
			slock_acquire(&t->key_lock);
			int val = tty_keyboard_count(rproc->t);
			/* release lock */
			slock_release(&t->key_lock);
			return val;

		case TIOCOUTQ:
			/* No output buffer right now. */
			return 0;
		case TCFLSH:
			if(i == TCIFLUSH) tty_clear_input(t);
			else if(i == TCOFLUSH);
			else if(i == TCIOFLUSH) tty_clear_input(t);
			break;

		case TIOCSCTTY:
			/* TODO:check to make sure rproc is a session leader*/
			/* TODO:make sure rproc doesn't already have a tty*/
			if(proc_tty_connected(t))
				proc_disconnect(rproc);

			proc_set_ctty(rproc, t);
			break;

		case TIOCNOTTY:
			/* TODO: send SIGHUP to all process on tty t.*/
			if(rproc->t == t)
				proc_disconnect(rproc);
			break;

		case TIOCGPGRP:
			if(ioctl_arg_ok(arg, sizeof(pid_t)))
				return -1;
			*pgidp = t->cpgid;
			break;
		case TIOCSPGRP:
			if(ioctl_arg_ok(arg, sizeof(pid_t)))
                                return -1;
                        t->cpgid = *pgidp;
			break;
		case TIOCGSID:
			/* TODO: make sure this is correct */
			if(ioctl_arg_ok(arg, sizeof(pid_t)))
				return -1;
			*sidp = rproc->sid;
			break;
		case TIOCEXCL:
			/* TODO: enforce exlusive mode */
			t->exclusive = 1;
			break;
		case TIOCGEXCL:
			if(ioctl_arg_ok(arg, sizeof(int)))
                                return -1;
			*ip = t->exclusive;
			break;
		case TIOCNXCL:
			t->exclusive = 0;
			break;
		case TIOCGETD:
			if(ioctl_arg_ok(arg, sizeof(int)))
                                return -1;
			*ip = t->term.c_line;
			break;
		case TIOCSETD:
			if(ioctl_arg_ok(arg, sizeof(int)))
                                return -1;
			t->term.c_line = *ip;
			break;
	
		case TCXONC: /* Suspend tty */
		case TCSBRK:
		case TCSBRKP:
		case TIOCSBRK:
		case TIOCCBRK:
			cprintf("Warning: %s used ioctl feature"
					" that has no effect: %d\n", 
					rproc->name, request);
			break;
		default:
			cprintf("Warning: ioctl request not implemented:"
					" 0x%x\n", request);
			return -1;
	}
	return 0;	
}

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

void tty_enable(tty_t t)
{
	t->active = 1;
	active = t;
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

void tty_putc(tty_t t, char c)
{
	if(t->type==TTY_TYPE_SERIAL)
	{
		if(t->active) serial_write(&c,1);
		return;
	}
	if(t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_COLOR)
	{
		uchar printable = 1;
		uint new_pos = t->cursor_pos;
		int x;
		switch(c)
		{
			case '\n':
				/* Move down a line */
				t->cursor_pos += CONSOLE_COLS - 1;
				t->cursor_pos -= t->cursor_pos %
					CONSOLE_COLS;
				printable = 0;
				break;
			case '\t':
				/* round cursor position */
				new_pos += 8;
				new_pos -= new_pos % 8;
				for(x = 0;t->cursor_pos < new_pos;x++)
					tty_putc(t, ' ');	
				return;

		}
		if(t->active && printable)
		{
			console_putc(t->cursor_pos, c, t->color, 
					t->type==TTY_TYPE_COLOR, 
					(uchar*)t->mem_start);
		}
		if(printable)
		{
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
		}

		if(t->cursor_pos >= CONSOLE_COLS * CONSOLE_ROWS)
		{
			tty_scroll(t);
			t->cursor_pos = (CONSOLE_COLS * CONSOLE_ROWS)
				- CONSOLE_COLS;
		}

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
	uint bpr = CONSOLE_COLS * 2;
	uint rows = CONSOLE_ROWS;
	char* buffer = t->buffer;

	int x;
	for(x = 0;x < rows - 1;x++)
	{
		char* dst_row = buffer + x * bpr;
		char* src_row = dst_row + bpr;
		memmove(dst_row, src_row, bpr);
	}
	memset(buffer + x * bpr, 0, bpr);
	tty_print_screen(t, t->buffer);
}

void tty_printf(tty_t t, char* fmt, ...)
{
	va_list list;
	va_start(&list, (void**)&fmt);
	char buffer[1024];
	memset(buffer, 0, 1024);
	va_snprintf(buffer, 1024, &list, fmt);
	if(t->type==TTY_TYPE_COLOR||t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_SERIAL)
	{
		int i;
		for(i=0; i <strlen(buffer); i++)
		{
			tty_putc(t, buffer[i]);
		}
	}
	else
	{
		panic("TTY invalid graphics mode");
	}

	va_end(&list);
}

void tty_print_screen(tty_t t, char* buffer)
{
	if(t->type==TTY_TYPE_SERIAL)
	{
		return;
	}
	else if(t->type==TTY_TYPE_MONO||t->type==TTY_TYPE_COLOR)
	{
		char* vid_addr = (char*)t->mem_start;
		//char* graph_addr = NULL;
		uint sz = 0;
		if(t->type==TTY_TYPE_COLOR)
		{
			//graph_addr = CONSOLE_COLOR_BASE;
			sz = CONSOLE_ROWS*CONSOLE_COLS*2;
		}
		else
		{
			//graph_addr = CONSOLE_MONO_BASE;
			sz = CONSOLE_ROWS*CONSOLE_COLS;
		}
		memmove(vid_addr, buffer, sz);
		if(t->active)
		{
			//memmove(graph_addr, buffer, sz);
		}

	}
	else
	{
		panic("TTY invalid graphics mode");
	}
}

int tty_gets(char* dst, int sz)
{
	return block_keyboard_io(dst, sz);
}

char tty_getc(tty_t t)
{
	char c;
	if(block_keyboard_io(&c, 1) != 1)
		panic("tty: get char failed.\n");
	return c;
}

uchar tty_set_cursor(tty_t t, uchar enabled)
{
	t->cursor_enabled = enabled;
	return 0;
}

uchar tty_set_cursor_pos(tty_t t, uchar pos)
{
	t->cursor_pos = pos;
	return 0;
}

void tty_set_color(tty_t t, uchar color)
{
	t->color = color;
}

void tty_delete_char(tty_t t)
{
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
			break;
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
	slock_acquire(&t->key_lock);
	memset(&t->kbd_line, 0, sizeof(struct kbd_buff));
	memset(&t->keyboard, 0, sizeof(struct kbd_buff));
	slock_release(&t->key_lock);
}

extern slock_t ptable_lock;
void tty_handle_keyboard_interrupt(void)
{
	if(!active) 
	{
		cprintf("tty: there is no active tty.\n");
		return;
	}

	slock_acquire(&ptable_lock);
	slock_acquire(&active->key_lock);
	char c = 0;
	do
	{
		switch(active->type)
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

		//cprintf("Got character: %c\n", c);
		/* Preprocess the input (canonical mode only!) */
		uchar canon = active->term.c_lflag & ICANON;
		//cprintf("Canonical mode: %d\n", canon);
		if(canon)
		{
			if(c == 13)
			{
				/* Carrige return */
				if(active->term.c_iflag & IGNCR) continue;
				else if(active->term.c_iflag & ICRNL) c = '\n';
			} else if(c == '\n')
			{
				if(active->term.c_iflag & INLCR) 
					c = 13; /* Carriage return */

			}

			/* Check for case conversions */
			if(active->term.c_iflag & IUCLC)
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
				if(tty_keyboard_delete(active))
					tty_delete_char(active);
			} else {
				/* Write to the current line */
				tty_keyboard_write_buff(&active->kbd_line, c);
				if(active->kbd_line.key_nls)
				{
					/* A line delimiter was written */
					//cprintf("Signaled.\n");
					signal_keyboard_io(active);
				}
			}

			/* Check for echo */
			if(active->term.c_lflag & ECHO)
				tty_putc(active, c);
		} else {
			if(tty_keyboard_write_buff(&active->keyboard, c))
			{
				cprintf("Warning: keyboard buffer is full.\n");
			}
			/* signal anyone waiting for anything */
			signal_keyboard_io(active);

		}
		// cprintf("NLS in line buffer: %d\n", active->kbd_line.key_nls);
		// cprintf("Line buffer: %s\n", active->kbd_line.buffer +
		//		active->kbd_line.key_read);
	} while(1);


	slock_release(&ptable_lock);
	slock_release(&active->key_lock);
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
			!keyboard->key_full) return -1;

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
