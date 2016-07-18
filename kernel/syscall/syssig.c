#include <stdlib.h>
#include <string.h>

#include "panic.h"
#include "kstdlib.h"
#include "stdlock.h"
#include "pipe.h"
#include "file.h"
#include "devman.h"
#include "tty.h"
#include "fsman.h"
#include "proc.h"
#include "signal.h"
#include "syscall.h"

// #define DEBUG

#ifdef RELEASE
# undef DEBUG
#endif

int sys_sigaction(void)
{
	/* Assign the handler */
	int signum;
	struct sigaction* act;
	struct sigaction* old = NULL;

	if(syscall_get_int(&signum, 0))
		return -1;
	if(syscall_get_buffer_ptr((void**)&act, 
			sizeof(struct sigaction), 1))
		return -1;
	syscall_get_optional_ptr((void**)&old, 2);

	if(signum < 0 || signum >= NSIG)
		return -1;

#ifdef DEBUG
	cprintf("%s:%d: sigaction: for sig: %d\n",
		rproc->name, rproc->pid, signum);
#endif

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
#ifdef DEBUG
	cprintf("%s:%d: Sig proc mask: UNIMPLEMENTED.\n",
		rproc->name, rproc->pid);
#endif
	return 0;
}

int sys_sigpending(void)
{
#ifdef DEBUG
	cprintf("%s:%d: Sig pending: UNIMPLEMENTED.\n",
		rproc->name, rproc->pid);
#endif
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

#ifdef DEBUG
	cprintf("%s:%d: Setting signal handler for sig: %d\n",
		rproc->name, rproc->pid, signum);
#endif

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

#ifdef DEBUG
	cprintf("%s:%d: Waiting for signal...\n",
		rproc->name, rproc->pid);
#endif

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

#ifdef DEBUG
	cprintf("%s:%d: Sending signal %d to process %d\n",
		rproc->name, rproc->pid,
		sig, pid);
#endif

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
