#include <signal.h>

#include "panic.h"
#include "proc.h"
#include "chronos.h"

static int still_up()
{
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
		if(ptable[x].state != PROC_UNUSED)
			return 1;
	return 0;
}

void pre_shutdown()
{
	/* Prevent creation of new processes */
	slock_acquire(&ptable_lock);

	cprintf("kernel: sending SIGKILL to all processes...\n");
	/* Kill all processes */
	int i;
	for(i = 0;i < PTABLE_SIZE;i++)
	{
		if(ptable[i].state == PROC_UNUSED)
			continue;

		if(sig_proc(ptable + i, SIGKILL))
			panic("Couldn't kill process!\n");
	}

	/* Make sure all processes have exited */
	while(still_up())
	{
		yield_withlock();
	}

	cprintf("kernel: All processes killed.\n");
	cprintf("Syncing file systems...\t\t\t\t\t");
	fs_sync();
	cprintf("[ OK ]\n");
}
