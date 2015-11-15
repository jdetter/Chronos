#include <stdlib.h>
#include <string.h>

#include "kern/types.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "panic.h"
#include "tty.h"
#include "proc.h"

extern struct proc* rproc;

// #define DEBUG

static int tty_io_init(struct IODriver* driver);
static int tty_io_read(void* dst, uint start_read, size_t sz, void* context);
static int tty_io_write(void* src, uint start_write, size_t sz, void* context);
static int tty_io_ioctl(unsigned long request, void* arg, tty_t context);

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

static int tty_io_init(struct IODriver* driver)
{
	return 0;
}

static int tty_io_read(void* dst, uint start_read, size_t sz, void* context)
{
	sz = tty_gets(dst, sz);
	return sz;
}

static int tty_io_write(void* src, uint start_write, size_t sz, void* context)
{
	tty_t t = context;
	uchar* src_c = src;
	int x;
	for(x = 0;x < sz;x++, src_c++)
		tty_putc(t, *src_c);
	return sz;
}

static int tty_io_ioctl(unsigned long request, void* arg, tty_t t)
{
#ifdef DEBUG
	cprintf("tty: ioctl called: %d 0x%x\n", (int)request, (int)request);
#endif

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
			if(ioctl_arg_ok(arg, sizeof(struct termios)))
				return -1;
			memmove(arg, &t->term, sizeof(struct termios));
			break;
		case TCSETSF:
			if(ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			/* Clear the input buffer */
			tty_clear_input(t);
			break;
		case TCSETSW:
		case TCSETS:
			if(ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			memmove(&t->term, arg, sizeof(struct termios));
			break;
                case TCGETA:
                        if(ioctl_arg_ok(arg, sizeof(struct termio)))
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
			if(ioctl_arg_ok(arg, sizeof(struct termio)))
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
			if(ioctl_arg_ok(arg, sizeof(struct termio)))
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
			if(ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			if(t->termios_locked)
				memset(termiosp, 0xFF,sizeof(struct termios));
			else memset(termiosp, 0x00, sizeof(struct termios));
			break;
		case TIOCSLCKTRMIOS:
			if(ioctl_arg_ok(arg, sizeof(struct termios)))
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

#ifdef DEBUG
	cprintf("tty: ioctl successful!\n");
#endif
	return 0;	
}

