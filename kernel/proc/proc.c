/**
 * Authors: John Detter <john@detter.com>, Max Strange <mbstrange@wisc.edu>
 *
 * Functions for handling processes.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>

#include "kern/types.h"
#include "kern/stdlib.h"
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
#include "fpu.h"

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

struct proc* spawn_tty(tty_t t)
{
	/* Try to get a new process */
	struct proc* p = alloc_proc();
	if(!p) return NULL; /* Could we find an unused process? */

	/* Get the process table lock */
	slock_acquire(&ptable_lock);

	/* Setup the new process */
	p->t = t;
	p->pid = next_pid++;
	p->uid = 0; /* init is owned by root */
	p->gid = 0; /* group is also root */

	/* Setup stdin, stdout and stderr */
	if(fd_next(p) != 0) panic("spawn_tty: wrong fd for stdin\n");
	p->fdtab[0]->type = FD_TYPE_DEVICE;
	p->fdtab[0]->device = t->driver;
	if(fd_next(p) != 1) panic("spawn_tty: wrong fd for stdout\n");
	p->fdtab[1]->type = FD_TYPE_DEVICE;
	p->fdtab[1]->device = t->driver;
	if(fd_next(p) != 2) panic("spawn_tty: wrong fd for stderr\n");
	p->fdtab[2]->type = FD_TYPE_DEVICE;
	p->fdtab[2]->device = t->driver;

	p->stack_start = PGROUNDUP(UVM_TOP);
	p->stack_end = p->stack_start - PGSIZE;
	
	p->block_type = PROC_BLOCKED_NONE;
	p->b_condition = NULL;
	p->b_pid = 0;
	p->parent = p;

	strncpy(p->name, "init", MAX_PROC_NAME);
	strncpy(p->cwd, "/", MAX_PATH_LEN);

	/* Setup virtual memory */
	p->pgdir = (pgdir_t*)palloc();
	vm_copy_kvm(p->pgdir);

	vmflags_t dir_flags = VM_DIR_READ | VM_DIR_WRIT;
	vmflags_t tbl_flags = VM_TBL_READ | VM_TBL_WRIT;

	/* Map in a new kernel stack */
        vm_mappages(UVM_KSTACK_S, UVM_KSTACK_E - UVM_KSTACK_S, p->pgdir, 
		dir_flags, tbl_flags);
        p->k_stack = (char*)PGROUNDUP(UVM_KSTACK_E);
        p->tf = (struct trap_frame*)(p->k_stack - sizeof(struct trap_frame));
        p->tss = (struct task_segment*)(UVM_KSTACK_S);

	/* Map in a user stack. */
	vm_mappages(p->stack_end, PGSIZE, p->pgdir, 
		dir_flags | VM_DIR_USRP, tbl_flags | VM_TBL_USRP);

	/* Switch to user page table */
	switch_uvm(p);

	/* Basic values for the trap frame */
	/* Setup the user stack */
	uchar* ustack = (uchar*)p->stack_start;

	/* Fake env */
	ustack -= sizeof(int);
	*((uint*)ustack) = 0x0;

	/* Fake argv */
        ustack -= sizeof(int);
        *((uint*)ustack) = 0x0;

	/* argc = 0 */
        ustack -= sizeof(int);
        *((uint*)ustack) = 0x0;

	/* Fake eip */
        ustack -= sizeof(int);
        *((uint*)ustack) = 0xFFFFFFFF;
        p->tf->esp = (uint)ustack;

	/* Load the binary */
	uintptr_t code_start;
	uintptr_t code_end;
	uintptr_t entry = elf_load_binary_path("/bin/init", p->pgdir, 
		&code_start, &code_end, 1);
	p->code_start = code_start;
	p->code_end = code_end;
	p->entry_point = entry;
	p->heap_start = PGROUNDUP(code_end);
	p->heap_end = p->heap_start;

	/* Set the mmap area start */
	p->mmap_end = p->mmap_start = 
		PGROUNDUP(UVM_TOP) - UVM_MIN_STACK;

	/* Setup our floating point unit */
	fpu_reset();

	p->state = PROC_READY;
	slock_release(&ptable_lock);

	return p;
}

uchar proc_tty_connected(tty_t t)
{
	slock_acquire(&ptable_lock);
	uchar result = 0;
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].t == t)
		{
			result = 1;
			break;
		}
	}
	slock_release(&ptable_lock);
	return result;
}

void proc_disconnect(struct proc* p)
{
	slock_acquire(&ptable_lock);
	/* disconnect stdin, stdout and stderr */
	fd_free(p, 0);
	fd_free(p, 1);
	fd_free(p, 2);

	tty_t t = p->t;

	/* remove controlling terminal */
	p->t = NULL;
	slock_release(&ptable_lock);

	if(p->sid == p->sid)
		proc_tty_disconnect(t);
}

void proc_tty_disconnect(tty_t t)
{
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		if(ptable[x].t == t)
			proc_disconnect(ptable + x);
	}
}

void proc_set_ctty(struct proc* p, tty_t t)
{
	slock_acquire(&ptable_lock);
	rproc->t = t;
	if(fd_new(p, 0, 1) == -1) return;
	if(fd_new(p, 1, 1) == -1) return;
	if(fd_new(p, 2, 1) == -1) return;

	rproc->fdtab[0]->device = t->driver;
	rproc->fdtab[0]->type = FD_TYPE_DEVICE;
	rproc->fdtab[1]->device = t->driver;
	rproc->fdtab[1]->type = FD_TYPE_DEVICE;
	rproc->fdtab[2]->device = t->driver;
	rproc->fdtab[2]->type = FD_TYPE_DEVICE;
	slock_release(&ptable_lock);
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
