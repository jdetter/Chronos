#include "types.h"
#include "file.h"
#include "stdarg.h"
#include "stdlock.h"
#include "devman.h"
#include "tty.h"
#include "fsman.h"
#include "iosched.h"
#include "pipe.h"
#include "proc.h"
#include "panic.h"
#include "stdlib.h"
#include "cmos.h"
#include "rtc.h"
#include "ktime.h"

extern slock_t ptable_lock;
extern struct proc ptable[];
extern struct proc* rproc;

void iosched_check(void)
{
	tty_handle_keyboard_interrupt();
	/* Check for system time changes */
	uchar cmos_val = cmos_read_interrupt();
	if(cmos_val == 144)
	{
		/* Clock update */
		ktime_update();
	}
}

int block_keyboard_io(void* dst, int request_size)
{
	/* Acquire the ptable lock */
	slock_acquire(&ptable_lock);
	/* See if we can read now */
	slock_acquire(&rproc->t->key_lock);

	rproc->state = PROC_BLOCKED;
	rproc->block_type = PROC_BLOCKED_IO;
	rproc->io_type = PROC_IO_KEYBOARD;
	rproc->io_recieved = 0;
	rproc->io_request = request_size;
	rproc->io_dst = dst;
	memset(dst, 0, request_size);

	if(rproc->t->key_nls) 
	{
		signal_keyboard_io(rproc->t);
	} else {

		slock_release(&rproc->t->key_lock);
		yield_withlock();

		/* Reacuire all locks */
		/* Acquire the ptable lock */
		slock_acquire(&ptable_lock);
		/* See if we can read now */
		slock_acquire(&rproc->t->key_lock);
	}

	/**
	 * When we return here, we should have a partially filled buffer
	 * that ends with a new line character AND/OR a null character.
	 * We will also not have the ptable lock. If we recieved zero
	 * bytes there was a wakeup error.
	 */
	if(rproc->io_recieved == 0)
		panic("iosched: keyboard io wakeup error.\n");

	slock_release(&ptable_lock);
	slock_release(&rproc->t->key_lock);
	return rproc->io_recieved;
}

/* ptable and key lock must be held here. */
void signal_keyboard_io(tty_t t)
{
	/** 
	 * If there are no new line characters in the buffer, 
	 * We are not allowed to read.
	 */
	if(t->key_nls <= 0) return;

	/* Keyboard signaled. */
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].state == PROC_BLOCKED
				&& ptable[x].block_type == PROC_BLOCKED_IO
				&& ptable[x].io_type == PROC_IO_KEYBOARD
				&& ptable[x].t == t)
		{
			/* Found blocked process. */
			/* Read into the destination buffer */
			int read_bytes = ptable[x].io_recieved;
			int wake = 0;
			/* If there is a new line were done. */
			if(t->key_nls > 0) wake = 1;
			for(read_bytes = 0;read_bytes < 
					ptable[x].io_request;
					read_bytes++)
			{
				/* See if we have to wrap */
				if(ptable[x].t->key_read >= TTY_KEYBUFFER_SZ)
					ptable[x].t->key_read = 0;
				/* Check for empty */
				if(ptable[x].t->key_full &&
						ptable[x].t->key_write == 
						ptable[x].t->key_read) break;


				/* Read a character */
				char c = t->keyboard[t->key_read];
				/* Place character into dst */
				ptable[x].io_dst[read_bytes] = c;
				/* Increment read position */
				t->key_read++;
				/* update empty */
				t->key_full = 0;
				/* If c is a new line, were done. */
				if(c == '\n')  
				{
					t->key_nls--;
					wake = 1;
					read_bytes++;
					break;
				}

			}

			/* update recieved */
			ptable[x].io_recieved = read_bytes;

			if(wake)
			{
				ptable[x].state = PROC_RUNNABLE;
				ptable[x].block_type = PROC_BLOCKED_NONE;
				ptable[x].io_type = PROC_IO_NONE;
			}
			break;
		}
	}
}
