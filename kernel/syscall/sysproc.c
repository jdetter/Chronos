#include "types.h"
#include "syscall.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "x86.h"
#include "chronos.h"
#include "stdlib.h"
#include "vm.h"
#include "trap.h"
#include "panic.h"

extern pid_t next_pid;
extern slock_t ptable_lock;
extern struct proc* rproc;
extern struct proc ptable[];

void trap_return(void);
int sys_fork(void)
{
	struct proc* new_proc = alloc_proc();
	if(!new_proc) return -1;
	slock_acquire(&ptable_lock);
	new_proc->k_stack = rproc->k_stack;
	new_proc->pid = next_pid++;
	new_proc->uid = rproc->uid;
	new_proc->gid = rproc->gid;
	new_proc->parent = rproc;
	new_proc->heap_start = rproc->heap_start;
	new_proc->heap_end = rproc->heap_end;
	new_proc->stack_start = rproc->stack_start;
	new_proc->stack_end = rproc->stack_end;
	new_proc->state = PROC_RUNNABLE;
	new_proc->pgdir = (uint*) palloc();
	vm_copy_kvm(new_proc->pgdir);
	vm_copy_uvm(new_proc->pgdir, rproc->pgdir);
	new_proc->tf = rproc->tf;
	new_proc->t = rproc->t;
	new_proc->entry_point = rproc->entry_point;
	new_proc->tss = rproc->tss;
	new_proc->context = rproc->context;
	memmove(&new_proc->cwd, &rproc->cwd, MAX_PATH_LEN);
	memmove(&new_proc->name, &rproc->name, MAX_PROC_NAME);

	/* Copy all file descriptors */
	memmove(&new_proc->file_descriptors, &rproc->file_descriptors,
			sizeof(struct file_descriptor) * MAX_FILES);

	/* Increment file references for all inodes */
	int i;
	for(i = 0;i < MAX_FILES;i++)
	{
		switch(new_proc->file_descriptors[i].type)
		{
			case FD_TYPE_FILE:
				fs_add_inode_reference(new_proc->file_descriptors[i].i);
				break;
			case FD_TYPE_PIPE:
				if(rproc->file_descriptors[i].pipe_type == FD_PIPE_MODE_WRITE)
					rproc->file_descriptors[i].pipe->write_ref++;
				if(rproc->file_descriptors[i].pipe_type == FD_PIPE_MODE_READ)
					rproc->file_descriptors[i].pipe->read_ref++;
				break;
		}
	}

	/** 
	 * A quick note on the swap stack:
	 *
	 * The swap stack is an area in memory located just below the kernel
	 * stack for the running program. This are is used to map in another
	 * stack from another program. This is useful when you want to work
	 * on 2 stacks at the same time without changing page directories.
	 * Here we are using the swap stack to map in the stack of our new
	 * process and modifying the contents without changing page directories.
	 */

	/* Map the new process's stack into our swap space */
	vm_set_swap_stack(rproc->pgdir, new_proc->pgdir);

	struct trap_frame* tf = (struct trap_frame*)
		((uchar*)new_proc->tf - SVM_DISTANCE);
	tf->eax = 0; /* The child should return 0 */

	/* new proc needs a context */
	struct context* c = (struct context*)
		((uchar*)tf - sizeof(struct context));
	c->eip = (uint)trap_return;
	c->esp = (uint)tf - 4 + SVM_DISTANCE;
	c->cr0 = (uint)new_proc->pgdir;
	new_proc->context = (uint)c + SVM_DISTANCE;

	/* Clear the swap stack now */
	vm_clear_swap_stack(rproc->pgdir);

	/* release ptable lock */
	slock_release(&ptable_lock);

	return new_proc->pid;
}


/* int wait(int pid) */
int sys_wait(void)
{
	int pid;
	if(syscall_get_int(&pid, 0)) return -1;

	slock_acquire(&ptable_lock);

	int ret_pid = 0;
	struct proc* p = NULL;
	while(1)
	{
		int process;
		for(process = 0;process < PTABLE_SIZE;process++)
		{
			if(ptable[process].state == PROC_KILLED
					&& ptable[process].parent == rproc)
			{
				if(pid == -1 || ptable[process].pid == pid)
				{
					p = ptable + process;
					break;
				}
			}
		}

		if(p)
		{
			/* Harvest the child */
			ret_pid = p->pid;
			/* Free used memory */
			freepgdir(p->pgdir);

			/* change rproc to the child process so that we can close its files. */
			struct proc* current = rproc;
			rproc = p;
			/* Close open files */
			int file;
			for(file = 0;file < MAX_FILES;file++)
				close(file);
			rproc = current;

			memset(p, 0, sizeof(struct proc));
			p->state = PROC_UNUSED;
			break;
		} else {
			/* Lets block ourself */
			rproc->block_type = PROC_BLOCKED_WAIT;
			rproc->b_pid = pid;
			rproc->state = PROC_BLOCKED;
			/* Wait for a signal. */
			yield_withlock();
			/* Reacquire ptable lock */
			slock_acquire(&ptable_lock);
		}
	}

	/* Release the ptable lock */
	slock_release(&ptable_lock);

	return ret_pid;
}

/* int exec(const char* path, const char** argv) */
int sys_exec(void)
{
	const char* path;
	const char** argv;

	if(syscall_get_str_ptr(&path, 0)) return -1;;
	if(syscall_get_buffer_ptrs((uchar***)&argv, 1)) return -1;

	char cwd_tmp[MAX_PATH_LEN];
	memmove(cwd_tmp, rproc->cwd, MAX_PATH_LEN);
	/* Does the binary look ok? */
	if(check_binary(path))
	{
		/* see if it is available in /bin */
		strncpy(rproc->cwd, "/bin/", MAX_PATH_LEN);
		if(check_binary(path))
		{
			/* It really doesn't exist. */
			memmove(rproc->cwd, cwd_tmp, MAX_PATH_LEN);
			return -1;
		}
	}

	/* Create a copy of the path */
	char program_path[MAX_PATH_LEN];
	memset(program_path, 0, MAX_PATH_LEN);
	strncpy(program_path, path, MAX_PATH_LEN);
	/* acquire ptable lock */
	slock_acquire(&ptable_lock);

	/* Create argument array */
	char* args[MAX_ARG];
	memset(args, 0, MAX_ARG * sizeof(char*));

	/* Make sure the swap stack is empty */
	vm_clear_swap_stack(rproc->pgdir);

	/* Create a temporary stack and map it to our swap stack */
	uint stack_start = palloc();
	mappage(stack_start, SVM_KSTACK_S, rproc->pgdir, 1, 0);
	uchar* tmp_stack = (uchar*)SVM_KSTACK_S + PGSIZE;
	uint uvm_stack = PGROUNDUP(UVM_USTACK_TOP);
	/* copy arguments */
	int x;
	for(x = 0;argv[x];x++)
	{
		uint str_len = strlen(argv[x]) + 1;
		tmp_stack -= str_len;
		uvm_stack -= str_len;
		memmove(tmp_stack, (char*)argv[x], str_len);
		args[x] = (char*)uvm_stack;
	}

	/* Copy argument array */
	tmp_stack -= MAX_ARG * sizeof(char*);
	uvm_stack -= MAX_ARG * sizeof(char*);
	memmove(tmp_stack, args, MAX_ARG * sizeof(char*));
	uint arg_arr_ptr = uvm_stack;
	int arg_count = x;

	/* Push argv */
	tmp_stack -= 4;
	uvm_stack -= 4;
	*((uint*)tmp_stack) = arg_arr_ptr;

	/* push argc */
	tmp_stack -= 4;
	uvm_stack -= 4;
	*((uint*)tmp_stack) = (uint)arg_count;

	/* Add bogus return address */
	tmp_stack -= 4;
	uvm_stack -= 4;
	*((uint*)tmp_stack) = 0xFFFFFFFF;

	/* Free user memory */
	vm_free_uvm(rproc->pgdir);

	/* Invalidate the TLB */
	switch_uvm(rproc);

	/* load the binary if possible. */
	uint load_result = load_binary(program_path, rproc);
	if(load_result == 0)
	{
		pfree(stack_start);
		memmove(rproc->cwd, cwd_tmp, MAX_PATH_LEN);
		slock_release(&ptable_lock);
		return -1;
	}

	/* Change name */
	strncpy(rproc->name, program_path, FILE_MAX_PATH);

	/* Map the new stack in */
	mappage(stack_start, PGROUNDUP(UVM_USTACK_TOP) - PGSIZE,
			rproc->pgdir, 1, 0);
	/* Unmap the swap stack */
	vm_clear_swap_stack(rproc->pgdir);

	/* We now have the esp and ebp. */
	rproc->tf->esp = uvm_stack;
	rproc->tf->ebp = rproc->tf->esp;

	/* Set eip to correct entry point */
	rproc->tf->eip = rproc->entry_point;

	/* Adjust heap start and end */
	rproc->heap_start = PGROUNDUP(load_result);
	rproc->heap_end = rproc->heap_start;

	/* restore cwd */
	memmove(rproc->cwd, cwd_tmp, MAX_PATH_LEN);

	/* Release the ptable lock */
	slock_release(&ptable_lock);

	return 0;
}

int sys_execve(void)
{
	return -1;
}

int sys_getpid(void)
{
	return rproc->pid;
}

int sys_kill(void)
{
	return -1;
}

int sys_exit(void)
{
	/* Acquire the ptable lock */
	slock_acquire(&ptable_lock);

	/* Set state to killed */
	rproc->state = PROC_KILLED;

	/* Attempt to wakeup our parent */
	if(rproc->zombie == 0)
	{
		if(rproc->parent->block_type == PROC_BLOCKED_WAIT)
		{
			if(rproc->parent->b_pid == -1 || rproc->parent->b_pid == rproc->pid)
			{
				/* Our parent is waiting on us, release the block. */
				rproc->parent->block_type = PROC_BLOCKED_NONE;
				rproc->parent->state = PROC_RUNNABLE;
				rproc->parent->b_pid = 0;
			}
		}
	}

	/* Release ourself to the scheduler, never to return. */
	yield_withlock();
	return 0;
}

int sys__exit(void)
{
	slock_acquire(&ptable_lock);
	/* Set state to killed */
	rproc->state = PROC_KILLED;
	/* Clear all of the file descriptors (LEAK) */
	int x;
	for(x = 0;x < MAX_FILES;x++)
		rproc->file_descriptors[x].type = 0x0;

	/* release the lock */
	slock_release(&ptable_lock);

	/* Finish up with a call to exit() */
	sys_exit();
	/* sys_exit doesn't return */
	panic("sys_exit returned!\n");
	return 0;
}

/* int brk(void* addr) */
int sys_brk(void)
{
	void* addr;
	if(syscall_get_int((int*)&addr, 0), 0) return -1;

	slock_acquire(&ptable_lock);
	/* see if the address makes sense */
	uint check_addr = (uint)addr;
	if(PGROUNDUP(check_addr) >= rproc->stack_end
			|| check_addr < rproc->heap_start)
	{
		slock_release(&ptable_lock);
		return -1;
	}

	/* Change the address break */
	uint old = rproc->heap_end;
	rproc->heap_end = (uint)addr;
	/* Unmap pages */
	old = PGROUNDUP(old);
	uint page = PGROUNDUP(rproc->heap_end);
	for(;page != old;page += PGSIZE)
	{
		/* Free the page */
		uint pg = unmappage(page, rproc->pgdir);
		if(pg) pfree(pg);
	}

	/* Release lock */
	slock_release(&ptable_lock);
	return 0;
}

/* void* sys_sbrk(uint increment) */
int sys_sbrk(void)
{
	uint increment;
        if(syscall_get_int((int*)&increment, 0)) return -1;

	slock_acquire(&ptable_lock);
	uint old_end = rproc->heap_end;
	/* Will this collide with the stack? */
	if(PGROUNDUP(old_end + increment) == rproc->stack_end)
	{
		slock_release(&ptable_lock);
		return (int)NULL;
	}

	/* Map needed pages */
	mappages(old_end, increment, rproc->pgdir, 1);
	/* Change heap end */
	rproc->heap_end = old_end + increment;
	/* release lock */
	slock_release(&ptable_lock);

	/* return old end */
	return (int)old_end;
}

/* void* mmap(void* hint, uint sz, int protection,
		int flags, int fd, off_t offset) */
int sys_mmap(void)
{
	void* hint;
	uint sz;
	int protection;
	int flags;
	int fd;
	off_t offset;

	if(syscall_get_int((int*)&hint, 0)) return -1;
	if(syscall_get_int((int*)&sz, 1)) return -1;
	if(syscall_get_int(&protection, 2)) return -1;
	if(syscall_get_int(&flags,  3)) return -1;
	if(syscall_get_int(&fd, 4)) return -1;
	if(syscall_get_int((int*) &offset, 5)) return -1;
	
	slock_acquire(&ptable_lock);
	uint pagestart = PGROUNDUP(rproc->heap_end);
	mappages(pagestart, sz, rproc->pgdir, 1);
	rproc->heap_end += sz;
	slock_release(&ptable_lock);
	return (int)pagestart;
}

/* int chdir(const char* dir) */
int sys_chdir(void)
{
	const char* dir;
	if(syscall_get_str_ptr(&dir, 0)) return -1;
	
	if(strlen(dir) < 1) return -1;
	char dir_path[MAX_PATH_LEN];
	char tmp_path[MAX_PATH_LEN];
	memset(dir_path, 0, MAX_PATH_LEN);
	memset(tmp_path, 0, MAX_PATH_LEN);
	if(file_path_dir(dir, dir_path, MAX_PATH_LEN))
		return -1;
	if(fs_path_resolve(dir_path, tmp_path, MAX_PATH_LEN))
		return -1;
	if(strlen(tmp_path) < 1) return -1;
	if(file_path_dir(tmp_path, dir_path, MAX_PATH_LEN))
		return -1;

	/* See if it exists */
	inode i = fs_open(dir_path, O_RDONLY, 0x0, 0x0, 0x0);
	if(i == NULL) return -1;

	/* is it a directory? */
	struct stat st;
	fs_stat(i, &st);
	if(!S_ISDIR(st.st_mode))
	{
		fs_close(i);
		return -1;
	}

	fs_close(i);
	/* Everything is ok. */
	memmove(rproc->cwd, dir_path, MAX_PATH_LEN);
	return 0;
}

/* int cwd(char* dst, uint sz) */
int sys_cwd(void)
{
	char* dst;
	uint sz;	

	if(syscall_get_int((int*)&sz, 1)) return -1;
	if(syscall_get_buffer_ptr((void**)&dst, sz, 0)) return -1;
	strncpy(dst, rproc->cwd, sz);
	return sz;
}


