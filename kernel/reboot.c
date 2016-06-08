#include <signal.h>

#include "panic.h"
#include "proc.h"
#include "chronos.h"

#if 0
static int still_up()
{
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable + 1 == rproc) continue;

		if(ptable[x].state != PROC_UNUSED)
		{
			cprintf("Still alive: %s\n", ptable[x].name);
			return 1;
		}
	}

	return 0;
}
#endif

void pre_shutdown()
{
	/* Prevent creation of new processes */
	slock_acquire(&ptable_lock);

	cprintf("kernel: sending SIGKILL to all processes...\n");
#if 0
	/* Kill all processes */
	int i;
	for(i = 0;i < PTABLE_SIZE;i++)
	{
		if(ptable[i].state == PROC_UNUSED
				|| ptable + i == rproc)
			continue;

		if(sig_proc(ptable + i, SIGKILL))
			ic("Couldn't kill process!\n");
		cprintf("killed %s\n", ptable[i].name);
	}

	/* Make sure all processes have exited */
	i = 0;
	do
	{
		yield_withlock();
		i++;
	} while(still_up() && i < 1000);
#endif

	cprintf("kernel: All processes killed.\n");
	cprintf("Syncing file systems...\t\t\t\t\t");
	fs_sync();
	cprintf("[ OK ]\n");
}
