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
	tty_t t = context;
	uchar* dst_c = dst;
	int x;
	for(x = 0;x < sz;x++, dst_c++)
	{
		*dst_c = tty_getc(t);
		if(*dst_c == '\n') break;
	}
	return sz;
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
	int* ip = (uint)arg;
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
			memmove(&t->term.cc_cc, termiop->c_cc, NCCS);

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
			memmove(&t->term.cc_cc, termiop->c_cc, NCCS);
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
			memmove(&t->term.cc_cc, termiop->c_cc, NCCS);
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
			int read = t->key_read;
			int write = t->key_write;
			int val = 0;
			if(t->key_full)
			{
				val = TTY_KEYBUFFER_SZ;
				goto FIONREAD_done;
			} else if(read == write)
			{
				val = 0;
				goto FIONREAD_done;
			}

			/* if read < write then just return the diff */
			if(read < write)
			{
				val = write - read;
				goto FIONREAD_done;
			} 
			/* If read > write, then wrap */
			val = TTY_KEYBUFFER_SZ - read + write;
FIONREAD_done:
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
			*sidp = t->rproc->sid;
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

	/* Zero keyboard status */
	memset(t->keyboard, 0, TTY_KEYBUFFER_SZ);
	t->key_write = 0;
	t->key_read = 0;
	t->key_full = 0;	
	t->key_nls = 0;

	/* Set the window spec */
	t->window.ws_row = CONSOLE_ROWS;
	t->window.ws_col = CONSOLE_COLS;
	slock_init(&t->key_lock);
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
	memset(t->keyboard, 0, TTY_KEYBUFFER_SZ);
	t->key_write = 0;
	t->key_read = 0;
	t->key_full = 0;
	t->key_nls = 0;
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
		/* If the buffer is full, break */
		if(active->key_full) 
		{
			cprintf("tty: keyboard buffer is full!\n");
			break;
		}
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

		switch(c)
		{
			/* replace line feed with nl */
			case '\n': case 13: 
				c = '\n';
				active->key_nls++;
				break;

				/* Delete is special */
				/* Delete is not a character */
			case 0x7F: case 0x08:
				/* see if the buffer is empty */
				if(active->key_read == 
						active->key_write &&
						!active->key_full) 
					continue;

				int tmp_write = 
					active->key_write - 1;
				if(tmp_write < 0)
					tmp_write = 
						TTY_KEYBUFFER_SZ - 1;
				/* Can't delete a nl */
				if(active->keyboard[tmp_write]
						== '\n') continue;

				/* Delete the character */
				active->keyboard[tmp_write]= 0;
				tty_delete_char(active);
				active->key_write = tmp_write;
				continue;
		}

		/* Echo to tty */
		tty_putc(active, c);


		/* put c into the buffer */
		/* wrap if needed */
		if(active->key_write >= TTY_KEYBUFFER_SZ)
			active->key_write = 0;
		active->keyboard[active->key_write] = c;
		active->key_write++;
		/* update full */
		if(active->key_write == active->key_read)
			active->key_full = 1;
		/* If that was a nl, we want to signal */
		if(c == '\n')
			signal_keyboard_io(active);
	} while(1);

	slock_release(&ptable_lock);
	slock_release(&active->key_lock);
}
