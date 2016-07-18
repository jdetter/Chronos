/**
 * Author: John Detter <john@detter.com>
 *
 * IO Scheduler.
 */

#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "proc.h"
#include "panic.h"
#include "drivers/cmos.h"
#include "drivers/rtc.h"
#include "ktime.h"

// #define DEBUG

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

void iosched_check_sleep(void)
{
	slock_acquire(&ptable_lock);
	time_t time = ktime_seconds();

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
