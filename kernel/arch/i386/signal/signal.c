/**
 * Author: John Detter <john@detter.com>
 *
 * Functions for handling signaling processes.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "kstdlib.h"
#include "ksignal.h"
#include "x86.h"
#include "stdarg.h"
#include "stdlock.h"
#include "trap.h"
#include "file.h"
#include "fsman.h"
#include "devman.h"
#include "pipe.h"
#include "tty.h"
#include "proc.h"
#include "panic.h"
#include "vm.h"
#include "cpu.h"
#include "chronos.h"

//#define DEBUG

/* Signal default action table */
static int default_actions[NSIG];

void* mmap(void* hint, uint sz, int protection,
        int flags, int fd, off_t offset);

/* Signal table */
static struct signal_t sigtable[SIG_TABLE_SZ];
static slock_t sigtable_lock;

static struct signal_t* sig_alloc(void)
{
	slock_acquire(&sigtable_lock);
	
	struct signal_t* sig = NULL;
	int x;
	for(x = 0;x < NSIG;x++)
	{
		if(!sigtable[x].allocated)
		{
			memset(sigtable + x, 0, sizeof(struct signal_t));
			sigtable[x].allocated = 1;
			sig = sigtable + x;
			break;
		}
	}

	if(!sig)
		cprintf("kernel: warning: signal table full.\n");

	slock_release(&sigtable_lock);

	return sig;
}

static void sig_free(struct signal_t* sig)
{
	slock_acquire(&sigtable_lock);
	if(!sig) panic("kernel: tried to free NULL signal.\n");
	if(sig < sigtable || sig >= sigtable + NSIG)
		panic("kernel: tried to free invalid signal.\n");
	sig->allocated = 0;
	slock_release(&sigtable_lock);
}

void sig_init(void)
{
	slock_init(&sigtable_lock);
	memset(sigtable, 0, sizeof(struct signal_t) * NSIG);

	int x;
	for(x = 0;x < NSIG;x++)
		default_actions[x] = SIGDEFAULT_KPANIC;

	default_actions[SIGHUP]   = SIGDEFAULT_TERM;
	default_actions[SIGINT]   = SIGDEFAULT_TERM;
	default_actions[SIGQUIT]  = SIGDEFAULT_CORE;
	default_actions[SIGILL]   = SIGDEFAULT_CORE;
	default_actions[SIGTRAP]  = SIGDEFAULT_CORE;
	default_actions[SIGIOT]   = SIGDEFAULT_TERM;
	default_actions[SIGABRT]  = SIGDEFAULT_CORE;
	default_actions[SIGEMT]   = SIGDEFAULT_TERM;
	default_actions[SIGFPE]   = SIGDEFAULT_CORE;
	default_actions[SIGKILL]  = SIGDEFAULT_TERM;
	default_actions[SIGBUS]   = SIGDEFAULT_CORE;
	default_actions[SIGSEGV]  = SIGDEFAULT_CORE;
	default_actions[SIGSYS]   = SIGDEFAULT_CORE;
	default_actions[SIGPIPE]  = SIGDEFAULT_TERM;
	default_actions[SIGALRM]  = SIGDEFAULT_TERM;
	default_actions[SIGTERM]  = SIGDEFAULT_TERM;
	default_actions[SIGURG]   = SIGDEFAULT_IGN;
	default_actions[SIGSTOP]  = SIGDEFAULT_STOP;
	default_actions[SIGTSTP]  = SIGDEFAULT_STOP;
	default_actions[SIGCONT]  = SIGDEFAULT_CONT;
	default_actions[SIGCHLD]  = SIGDEFAULT_IGN;
	default_actions[SIGTTIN]  = SIGDEFAULT_STOP;
	default_actions[SIGTTOU]  = SIGDEFAULT_STOP;
	default_actions[SIGIO]    = SIGDEFAULT_TERM;
	default_actions[SIGWINCH] = SIGDEFAULT_IGN;
	default_actions[SIGUSR1]  = SIGDEFAULT_TERM;
	default_actions[SIGUSR2]  = SIGDEFAULT_TERM;
}

static void sig_queue(struct proc* p, struct signal_t* sig)
{
	struct signal_t* ptr = p->sig_queue;
	if(!ptr)
	{
		p->sig_queue = sig;
		return;
	}

	if(sig->catchable) /* Add to the end */
	{
		while(ptr->next) ptr = ptr->next;
		ptr->next = sig;
	} else { /* High priority signal, add to front. */
		sig->next = ptr;
		p->sig_queue = sig;
	}
}

static void sig_dequeue(struct proc* p)
{
	struct signal_t* ptr = p->sig_queue;
	if(ptr)
	{ 
		p->sig_queue = ptr->next;
		sig_free(ptr);
	}
}

int sig_clear(struct proc* p)
{
	while(p->sig_queue) sig_dequeue(p);
	return 0;
}

int sig_proc(struct proc* p, int sig)
{
	if(sig < 0 || sig >= NSIG)
		return -1;
	if(!p) return -1;

	/* Get a new signal */
	struct signal_t* signal = sig_alloc();
	if(!signal) return -1;

	/* Setup signal */
	signal->signum = sig;
	signal->default_action = default_actions[sig];
	signal->next = NULL;

	if(sig != SIGKILL && sig != SIGSTOP)
		signal->catchable = 1;

	/* Queue up this signal */
	sig_queue(p, signal);

	/* Check to see if a program needs to be woken up. */
	if(signal->default_action == SIGDEFAULT_CONT
			&& p->state == PROC_STOPPED)
		p->state = PROC_RUNNABLE;

	if(p->state == PROC_SIGNAL || sig == SIGKILL)
		p->state = PROC_RUNNABLE;

	return 0;
}

int sig_cleanup(void)
{
	/* Check to make sure there was a signal to cleanup */
	if(!rproc->sig_queue || !rproc->sig_handling) return -1;

	/* Restore trap frame */
	memmove((char*)rproc->k_stack - sizeof(struct trap_frame), 
			&rproc->sig_saved, 
			sizeof(struct trap_frame));
	sig_dequeue(rproc);
	return 0;
}

int sig_handle(void)
{
	/* Make sure there is a signal being waited on. */
	if(!rproc->sig_queue)
		return 0;

	/* Not sure if interrupts will mess this up but better be sure */
	push_cli();

	/* Get the signal from the top of the queue */
	struct signal_t* sig = rproc->sig_queue;
	void (*sig_handler)(int sig_num) = 
		rproc->sigactions[sig->signum].sa_handler;
	int caught = 0; /* Did we catch the signal? */
	int terminated = 0; /* Did we get terminated? */
	int stopped = 0; /* Did we get stopped? */
	int core = 0; /* Should we dump the core? */
	int ignored = 0; /* Was the default action caught and ignored? */

	/* Does the user want us to use the default action? */
	if(rproc->sigactions[sig->signum].sa_handler == SIG_DFL)
		sig->catchable = 0; /* Make signal uncatchable */

	switch(rproc->sig_queue->default_action)
	{
		case SIGDEFAULT_KPANIC:
			panic("kernel: Invalid signal handled!\n");
			break;
		case SIGDEFAULT_TERM:
			if(sig->catchable)
			{
				caught = 1;
			} else {
				terminated = 1;
			}
			break;
		case SIGDEFAULT_CORE:
			if(sig->catchable)
			{
				caught = 1;
			} else { 
				core = terminated = 1;
			}
			break;
		case SIGDEFAULT_STOP:
			if(sig->catchable)
			{
				caught = 1;
			} else {
				stopped = 1;
			}
			break;
		case SIGDEFAULT_CONT:
			if(sig->catchable)
			{
				caught = 1;
			}
			break;
		case SIGDEFAULT_IGN:
			if(sig->catchable)
			{
				caught = 1;
			}
			break;
	}

	/** 
	 * If the user wants this signal to be ignored, ignore it. 
	 * EXCEPTIONS: if we are terminated, dumped or stopped then
	 * we must continue with the default action.
	 */
	if(rproc->sigactions[sig->signum].sa_handler == SIG_IGN)
	{
		if(!terminated && !stopped && !core)
			ignored = 1;
		caught = 0;
	}

	/* Were we able to catch the signal? */
	if(caught)
	{
		/* Do we have a signal stack? */
		if(!rproc->sig_stack_start)
		{
			/* TODO: probably don't use mmap here, this will */
			/*        change in the future.                  */

			/* Allocate a signal stack */
			int pages = SIG_DEFAULT_STACK + SIG_DEFAULT_GUARD;
			uint end = (uint)mmap(NULL, 
					pages * PGSIZE, 
					PROT_WRITE | PROT_READ,
					MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

			/* Unmap all guard pages */
			int x;
			for(x = 0;x < SIG_DEFAULT_GUARD;x++)
			{
				vm_unmappage(end, rproc->pgdir);
				end += PGSIZE;
			}

			/* Set sig stack start */
			rproc->sig_stack_start = end + 
				(SIG_DEFAULT_STACK * PGSIZE);
		}

		/* Save the current trap frame if we need to */
		if(!rproc->sig_handling)
		{
			/* We weren't handling a signal until now */
			memmove(&rproc->sig_saved, 
					(char*)rproc->k_stack - sizeof(struct trap_frame),
					sizeof(struct trap_frame));
		}

		pstack_t stack = rproc->sig_stack_start;

		/* set the return address to the sig handler */
		rproc->tf->eip = (uintptr_t)sig_handler;

		vmflags_t dir_flags = VM_DIR_USRP | VM_DIR_READ | VM_DIR_WRIT;
		vmflags_t tbl_flags = VM_TBL_USRP | VM_TBL_READ | VM_TBL_WRIT;

		/* Push argument (sig) */
		stack -= sizeof(int);
		vm_memmove((void*)stack, &sig->signum, sizeof(int),
				rproc->pgdir, rproc->pgdir,
				dir_flags, tbl_flags);

		/* (safely) Push our magic return value */
		stack -= sizeof(int);
		uint32_t mag = SIG_MAGIC;
		vm_memmove((void*)stack, &mag, sizeof(int), 
				rproc->pgdir, rproc->pgdir,
				dir_flags, tbl_flags);

		/* Update stack pointer */
		rproc->tf->esp = stack;

		/* We are now actively handling this signal */
		rproc->sig_handling = 1;
	}

	pop_cli(); 

	/* Check to see if we need to dump the core */
	if(core)
	{
		/* Dump memory for gdb analysis */
#ifdef DEBUG
		cprintf("%s:%d: CORE DUMPED\n", rproc->name, rproc->pid);
#endif
		rproc->status_changed = 1;
		rproc->return_code = ((sig->signum & 0xFF) << 0x8) | 0x02;
		rproc->state = PROC_ZOMBIE;
		wake_parent(rproc);
		yield_withlock();
	}

	/* If we got stopped, we will enter scheduler. */
	if(stopped)
	{
#ifdef DEBUG
		cprintf("%s:%d: process stopped.\n", rproc->name, rproc->pid);
#endif

		rproc->status_changed = 1;
		rproc->state = PROC_STOPPED;
		rproc->return_code = ((sig->signum & 0xFF) << 0x08) | 0x04;
		/* Should we wake the parent? */
		if(!rproc->orphan && rproc->parent
				&& (rproc->parent->wait_options & WUNTRACED))
		{
			wake_parent(rproc);
		}
		sig_dequeue(rproc);
		yield_withlock();

		/* Should we wake our parent now that we've continued */
		rproc->status_changed = 1;
		if(!rproc->orphan && rproc->parent
				&& (rproc->parent->wait_options & WCONTINUED))
		{
			wake_parent(rproc);
		}

	}

	/* Check to see if we got terminated */
	if(terminated)
	{
#ifdef DEBUG
		cprintf("%s:%d: Process killed by signal. No core dumped.\n", 
				rproc->name, rproc->pid);
#endif

		rproc->status_changed = 1;
		rproc->return_code = ((sig->signum & 0xFF) << 0x08) | 0x01;
		rproc->state = PROC_ZOMBIE;
		wake_parent(rproc);
		yield_withlock();
	}

	if(ignored)
		sig_dequeue(rproc);

	return 0;
}
