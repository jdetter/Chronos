/**
 * Authors: John Detter <john@detter.com>, Max Strange <mbstrange@wisc.edu>
 *
 * Functions for handling processes.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>

#include "kstdlib.h"
#include "file.h"
#include "elf.h"
#include "stdlock.h"
#include "devman.h"
#include "tty.h"
#include "file.h"
#include "fsman.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "trap.h"
#include "panic.h"
#include "stdarg.h"
#include "syscall.h"
#include "chronos.h"
#include "iosched.h"
#include "time.h"
#include "context.h"

extern struct vsfs_context context;

/* The process table lock must be acquired before accessing the ptable. */
slock_t ptable_lock;
/* The process table */
struct proc ptable[PTABLE_SIZE];
/* A pointer into the ptable of the running process */
struct proc* rproc;
/* The next available pid */
pid_t next_pid;
/* How many ticks have there been since boot? */
int k_ticks;

extern struct file_descriptor* fd_tables[PTABLE_SIZE][PROC_MAX_FDS];
extern slock_t fd_tables_locks[];

struct proc* alloc_proc()
{
	slock_acquire(&ptable_lock);
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].state == PROC_UNUSED)
		{
			memset(ptable + x, 0, sizeof(struct proc));
			ptable[x].state = PROC_EMBRYO;
			ptable[x].fdtab = fd_tables[x];
			ptable[x].fdtab_lock = &fd_tables_locks[x];
			break;
		}
	}

	slock_release(&ptable_lock);

	if(x >= PTABLE_SIZE) return NULL;
	return ptable + x;
}

struct proc* get_proc_pid(int pid)
{
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].pid == pid)
			return ptable + x;
	}

	/* There is no process with that pid. */
	return NULL;
}

void proc_print_table(void)
{
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(!ptable[x].state) continue;

		cprintf("%s %d\n", ptable[x].name, 
				ptable[x].pid);
		cprintf("Open Files\n");
		int fd;
		for(fd = 0;fd < PROC_MAX_FDS;fd++)
		{
			if(!ptable[x].fdtab[fd]) continue;
			if(!ptable[x].fdtab[fd]->type)
				continue;
			cprintf("\t%d: name: %s refs: %d\n", 
				fd, ptable[x].fdtab[fd]->path,
				ptable[x].fdtab[fd]->refs);
		}
		cprintf("Working directory: %s\n", 
				ptable[x].cwd);
	}
}
