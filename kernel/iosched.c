/**
 * Author: John Detter <john@detter.com>
 *
 * IO Scheduler.
 */

#include <stdlib.h>
#include <string.h>

#include "kern/types.h"
#include "kern/stdlib.h"
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
#include "cmos.h"
#include "rtc.h"
#include "ktime.h"

extern slock_t ptable_lock;
extern struct proc ptable[];
extern struct proc* rproc;

/**
 * Check to see if there are any processes that need to be woken up.
 */
void iosched_check_sleep(void);

void iosched_check(void)
{
	tty_keyboard_interrupt_handler();
	/* Check for system time changes */
	uchar cmos_val = cmos_read_interrupt();
	if(cmos_val == 144)
	{
		/* Clock update */
		ktime_update();

		/* Temporary: sync file system every second */
		fs_sync();
		//cprintf("File system synced.\n");
	}

	iosched_check_sleep();
}

#if 0
int DEPRICATED_block_keyboard_io(void* dst, int request_size)
{
	// cprintf("block called.\n");
	/* quick sanity check */
	if(!request_size || !dst) return 0;
	/* Acquire the ptable lock */
	slock_acquire(&ptable_lock);
	/* See if we can read now */
	slock_acquire(&rproc->t->key_lock);

	rproc->io_dst = dst;
	memset(dst, 0, request_size);

	/* Try to read what we can now */
	/* NOTE: this takes in account for canonical mode already. */
	int read_chars = 0;
	int c;
	while(read_chars < request_size  &&
		(c = tty_keyboard_read(rproc->t)))
	{
		rproc->io_dst[read_chars] = c;
		// cprintf("Quick read character: %c\n", c);
		read_chars++;
	}

	/* If we read anything at all, we need to return (mode independant)*/
	if(read_chars)
	{
		/* for completeness, set io_recieved */
		rproc->io_recieved = read_chars;

		/* release locks */
		slock_release(&ptable_lock);
		slock_release(&rproc->t->key_lock);
		return read_chars;
	}

	/* We didn't read anything, so we have to block. */
	// cprintf("Couldn't quick read any characters.\n");
keyboard_sleep:
	rproc->state = PROC_BLOCKED;
	rproc->block_type = PROC_BLOCKED_IO;
	rproc->io_type = PROC_IO_KEYBOARD;
	rproc->io_recieved = 0;
	rproc->io_request = request_size;

	// cprintf("Yielding...\n");
	slock_release(&rproc->t->key_lock);
	yield_withlock();

	/* Reacuire all locks */
	/* Acquire the ptable lock */
	slock_acquire(&ptable_lock);
	/* See if we can read now */
	slock_acquire(&rproc->t->key_lock);

	/**
	 * When we return here, we should have a partially filled buffer.
	 * If nothing was read, we need to go back to sleep.
	 */
	if(rproc->io_recieved == 0)
	{
		cprintf("iosched: keyboard io wakeup error.\n");
		goto keyboard_sleep;
	}

	slock_release(&ptable_lock);
	slock_release(&rproc->t->key_lock);
	return rproc->io_recieved;
}

/* ptable and key lock must be held here. */
void DEPRICATED_signal_keyboard_io(tty_t t)
{
	char canon = t->term.c_lflag & ICANON;

	/* Keyboard signaled. */
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].state == PROC_BLOCKED
				&& ptable[x].block_type == PROC_BLOCKED_IO
				&& ptable[x].io_type == PROC_IO_KEYBOARD
				&& ptable[x].t == t)
		{
			// cprintf("Found blocked process: %s\n", ptable[x].name);
			/* Acquire the key lock */
			/* Found blocked process. */
			/* Read into the destination buffer */
			int read_bytes = ptable[x].io_recieved;
			int wake = 0; /* Wether or not to wake the process */

			for(read_bytes = 0;read_bytes < 
					ptable[x].io_request;
					read_bytes++)
			{
				/* Read a character */
				char c = tty_keyboard_read(t);
				if(!c) break;
				/* Place character into dst */
				ptable[x].io_dst[read_bytes] = c;
				// cprintf("Read: %c\n", c);
				if(canon && (c == '\n' || c == 13))
				{
					read_bytes++;
					break;
				}
			}

			/* If we read anything, wakeup the process */
			if(read_bytes > 0) wake = 1;

			/* update recieved */
			ptable[x].io_recieved = read_bytes;

			if(wake)
			{
				// cprintf("Process unblocked.\n");
				ptable[x].state = PROC_RUNNABLE;
				ptable[x].block_type = PROC_BLOCKED_NONE;
				ptable[x].io_type = PROC_IO_NONE;
			}

			break;
		}
	}
}

#endif

void iosched_check_sleep(void)
{
	slock_acquire(&ptable_lock);
	uint time = ktime_seconds();

	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].state == PROC_BLOCKED &&
				ptable[x].block_type == PROC_BLOCKED_SLEEP &&
				ptable[x].sleep_time <= time)
		{
			ptable[x].state = PROC_RUNNABLE;
			ptable[x].block_type = 0x0;
			ptable[x].sleep_time = 0x0;
		}
	}

	slock_release(&ptable_lock);
}
