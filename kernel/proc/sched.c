#include <string.h>

#include "iosched.h"
#include "stdlock.h"
#include "proc.h"
#include "devman.h"
#include "context.h"
#include "panic.h"

void sched_init()
{
	/* Zero all of the processes (unused) */
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
		memset(ptable + x, 0, sizeof(struct proc));
	/* No process is running right now. */
	rproc = NULL;
	/* Initilize our process table lock */
	slock_init(&ptable_lock);
}

void yield(void)
{
	/* We are about to enter the scheduler again, reacquire lock. */
	slock_acquire(&ptable_lock);

	/* Set state to runnable. */
	rproc->state = PROC_RUNNABLE;

	/* Give up cpu for a scheduling round */
	context_restore((uintptr_t*)&rproc->context, k_context);

	/* When we get back here, we no longer have the ptable lock. */
}

void yield_withlock(void)
{
	/* We have the lock, just enter the scheduler. */
	/* We are also not changing the state of the process here. */

	/* Give up cpu for a scheduling round */
	context_restore((uintptr_t*)&rproc->context, k_context);

	/* When we get back here, we no longer have the ptable lock. */
}

void sched(void)
{
	/* Acquire ptable lock */
	slock_acquire(&ptable_lock);
	scheduler();	
}

void scheduler(void)
{
	/* WARNING: ptable lock must be held here.*/

	while(1)
	{
		int x;
		for(x = 0;x < PTABLE_SIZE;x++)
		{
			if(ptable[x].state == PROC_RUNNABLE
					|| ptable[x].state == PROC_READY)
			{
				/* Found a process! */
				rproc = ptable + x;

				/* release lock */
				slock_release(&ptable_lock);

				/* Make the context switch */
				context_switch(rproc);
				//cprintf("Process is done for now.\n");

				// The new process is now scheduled.
				/* The process is done for now. */
				rproc = NULL;

				/* The process has reacquired the lock. */
			}
		}

		/* We still have the process table lock */
		slock_release(&ptable_lock);
		/* run io scheduler */
		iosched_check();
		/* Reacquire the lock */
		slock_acquire(&ptable_lock);
	}
}
