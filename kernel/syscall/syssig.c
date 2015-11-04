#include <stdlib.h>
#include <string.h>

#include "kern/types.h"
#include "kern/stdlib.h"
#include "stdlock.h"
#include "pipe.h"
#include "file.h"
#include "devman.h"
#include "tty.h"
#include "fsman.h"
#include "proc.h"
#include "signal.h"
#include "syscall.h"

extern slock_t ptable_lock;
extern struct proc ptable[];
extern struct proc* rproc;

int sys_sigaction(void)
{
	/* Assign the handler */
	int signum;
	struct sigaction* act;
	struct sigaction* old;

	if(syscall_get_int(&signum, 0))
		return -1;
	if(syscall_get_buffer_ptr((void**)&act, 
			sizeof(struct sigaction),
			1))
		return -1;
	if(syscall_get_int((int*)&old, 2))
		return -1;

	if(old != NULL && syscall_get_buffer_ptr((void**)&old,
			sizeof(struct sigaction), 2))
		return -1;

	if(signum < 0 || signum >= NSIG)
		return -1;

	slock_acquire(&ptable_lock);
	if(old)
	{
		memmove(old, &rproc->sigactions[signum],
                	sizeof(struct sigaction));
	}
	memmove(&rproc->sigactions[signum],
		act, sizeof(struct sigaction));
	
	slock_release(&ptable_lock);

	return 0;
}

int sys_sigprocmask(void)
{
	return 0;
}

int sys_sigpending(void)
{
	return 0;
}

int sys_signal(void)
{
	int signum;
	struct sigaction* act;

	if(syscall_get_int(&signum, 0))
		return -1;

	if(syscall_get_int((int*)&act, 1))
		return -1;

	if(act != (struct sigaction*)SIG_IGN 
		&& act != (struct sigaction*)SIG_DFL 
		&& act != (struct sigaction*)SIG_ERR)
	{
		if(syscall_get_buffer_ptr((void**)&act,
					sizeof(struct sigaction), 1))
			return -1;
	}

	if(signum < 0 || signum >= NSIG)
		return -1;

	slock_acquire(&ptable_lock);
	if(act == (struct sigaction*)SIG_IGN 
                || act == (struct sigaction*)SIG_DFL 
                || act == (struct sigaction*)SIG_ERR)
	{
		rproc->sigactions[signum].sa_handler = (void*)act;
	} else {
		memmove(&rproc->sigactions[signum],
				act, sizeof(struct sigaction));
	}
	slock_release(&ptable_lock);

	return (int)act;
}

int sys_sigsuspend(void)
{
	sigset_t set;

	if(syscall_get_int((int*)&set, 0))
		return -1;

	slock_acquire(&ptable_lock);

	rproc->sig_suspend_mask = set;
	rproc->state = PROC_SIGNAL;
	yield_withlock();

	slock_release(&ptable_lock);

	return 0;
}

int sys_kill(void)
{
	pid_t pid;
	int sig;

	if(syscall_get_int(&pid, 0)) return -1;
	if(syscall_get_int(&sig, 1)) return -1;

	slock_acquire(&ptable_lock);

	if(pid > 0)
	{
		struct proc* p = get_proc_pid(pid);
		if(!p) goto bad;
		if(sig_proc(p, sig))
			goto bad;
	} else if(pid == 0)
	{
		int x;
		for(x = 0;x < PTABLE_SIZE;x++)
		{
			/* Send the signal to the group */
			if(ptable[x].state && ptable[x].pgid == rproc->pgid)
				sig_proc(ptable + x, sig);
		}
	} else if(pid == -1)
	{
		/* Send to all process that we have permission to */

	} else {
		/* Send to process -pid*/
		pid = -pid;
		struct proc* p = get_proc_pid(pid);
		if(!p) goto bad;
		if(sig_proc(p, sig))
			goto bad;
	}

	slock_release(&ptable_lock);
	return 0;
bad:
	slock_release(&ptable_lock);
	return -1;
}
