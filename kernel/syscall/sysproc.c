#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/wait.h>

#include "kern/types.h"
#include "kern/stdlib.h"
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
#include "vm.h"
#include "trap.h"
#include "panic.h"
#include "rtc.h"
#include "ktime.h"
#include "signal.h"
#include "context.h"
#include "elf.h"
#include "sched.h"

#define DEBUG

extern pid_t next_pid;
extern slock_t ptable_lock;
extern uint k_ticks;
extern struct proc* rproc;
extern struct proc ptable[];

int waitpid_nolock(int pid, int* status, int options);
void fork_return(void);
int clone(unsigned long flags, void* child_stack,
                void* ptid, void* ctid,
                struct pt_regs* regs);
int waitpid_nolock_noharvest(int pid);


int sys_fork(void)
{
	struct proc* new_proc = alloc_proc();
	if(!new_proc) return -1;
	slock_acquire(&ptable_lock);
	/* Save the fdtab and lock */
	fdtab_t fdtab = new_proc->fdtab;
	slock_t* fdtab_lock = new_proc->fdtab_lock;
	/* Copy the entire process */
	memmove(new_proc, rproc, sizeof(struct proc));
	new_proc->fdtab = fdtab;
	new_proc->fdtab_lock = fdtab_lock;
	new_proc->pid = next_pid++;
	new_proc->ppid = rproc->pid;
	new_proc->tid = new_proc->pid;
	new_proc->tgid = new_proc->pid;
	new_proc->next_tid = new_proc->pid + 1;
	new_proc->parent = rproc;
	new_proc->state = PROC_RUNNABLE;
	new_proc->pgdir = (pgdir_t*) palloc();
	vm_copy_kvm(new_proc->pgdir);

#ifndef _ALLOW_VM_SHARE_
	vm_copy_uvm(new_proc->pgdir, rproc->pgdir);
#else
	/* vm_copy_uvm(new_proc->pgdir, rproc->pgdir); */
	vm_uvm_cow(rproc->pgdir);
	/* Create mappings for the user land pages */
	vm_map_uvm(new_proc->pgdir, rproc->pgdir);
	/* Create a kernel stack */
	vm_cpy_user_kstack(new_proc->pgdir, rproc->pgdir);
#endif

	/* Copy the table (NO MAP) */
	fd_tab_copy(new_proc, rproc);

	/* Increment file references for all inodes */
	int i;
	for(i = 0;i < PROC_MAX_FDS;i++)
	{
		if(!fd_ok(i)) continue;
		switch(new_proc->fdtab[i]->type)
		{
			case FD_TYPE_FILE:
				fs_add_inode_reference(new_proc->fdtab[i]->i);
				break;
			case FD_TYPE_PIPE:
				if(rproc->fdtab[i]->pipe_type 
						== FD_PIPE_MODE_WRITE)
					rproc->fdtab[i]->pipe->write_ref++;
				if(rproc->fdtab[i]->pipe_type 
						== FD_PIPE_MODE_READ)
					rproc->fdtab[i]->pipe->read_ref++;
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
		((char*)new_proc->tf - SVM_DISTANCE);
	tf->eax = 0; /* The child should return 0 */

	/* new proc needs a context */
	struct context* c = (struct context*)
		((char*)tf - sizeof(struct context));
	c->eip = (uintptr_t)fork_return;
	c->esp = (uintptr_t)tf - 4 + SVM_DISTANCE;
	c->cr0 = (uintptr_t)new_proc->pgdir;
	new_proc->context = (uintptr_t)c + SVM_DISTANCE;

	/* Clear the swap stack now */
	vm_clear_swap_stack(rproc->pgdir);

	/* release ptable lock */
	slock_release(&ptable_lock);

	return new_proc->pid;
}

/* int clone(unsigned long flags, void* child_stack, 
 *		void* ptid, void* ctid,
 *		struct pt_regs* regs) */
int sys_clone(void)
{
	unsigned long flags;
	void* child_stack;
	void* ptid;
	void* ctid;
	struct pt_regs* regs;

	if(syscall_get_long((long*)&flags, 0)) return -1;
	int start_pos = sizeof(long) / sizeof(int);
	if(syscall_get_buffer_ptr(&child_stack, 0x1000, start_pos))
		return -1;
	if(syscall_get_buffer_ptr(&ptid, sizeof(pid_t), start_pos + 1))
		return -1;
	if(syscall_get_buffer_ptr(&ctid, sizeof(pid_t), start_pos + 2))
		return -1;
	if(syscall_get_buffer_ptr((void*)&regs, sizeof(struct pt_regs), 
				start_pos + 3))
		return -1;

	return clone(flags, child_stack, ptid, ctid, regs);
}

int clone(unsigned long flags, void* child_stack,
		void* ptid, void* ctid,
		struct pt_regs* regs)
{	
	struct proc* new_proc = alloc_proc();
	if(!new_proc) return -1;

#ifdef DEBUG
	cprintf("%d: creating thread.\n", rproc->pid);
	cprintf("%d has thread group %d\n", rproc->tgid);
#endif

	struct proc* main_proc = get_proc_pid(rproc->tgid);
	slock_acquire(&ptable_lock);
	/* Save the fdtab and lock */
	fdtab_t fdtab = new_proc->fdtab;
	slock_t* fdtab_lock = new_proc->fdtab_lock;
	/* Copy the entire process */
	memmove(new_proc, main_proc, sizeof(struct proc));
	new_proc->state = PROC_EMBRYO;
	new_proc->fdtab = fdtab;
	new_proc->fdtab_lock = fdtab_lock;
	new_proc->pid = next_pid++;
	new_proc->tid = main_proc->next_tid++;
	new_proc->parent = main_proc;

#ifndef _ALLOW_VM_SHARE_
	/* Create a new page directory */
	new_proc->pgdir = (pgdir_t*)palloc();
	vm_copy_kvm(new_proc->pgdir);
	vm_copy_uvm(new_proc->pgdir, rproc->pgdir);

	/* Copy the table (NO MAP) */
	fd_tab_copy(new_proc, rproc);

	/* Increment file references for all inodes */
	int i;
	for(i = 0;i < PROC_MAX_FDS;i++)
	{
		if(!fd_ok(i)) continue;
		switch(new_proc->fdtab[i]->type)
		{
			case FD_TYPE_FILE:
				fs_add_inode_reference(new_proc->fdtab[i]->i);
				break;
			case FD_TYPE_PIPE:
				if(rproc->fdtab[i]->pipe_type 
						== FD_PIPE_MODE_WRITE)
					rproc->fdtab[i]->pipe->write_ref++;
				if(rproc->fdtab[i]->pipe_type 
						== FD_PIPE_MODE_READ)
					rproc->fdtab[i]->pipe->read_ref++;
				break;
		}
	}
#endif

	/* Create a copy of the kernel stack */
	/* Map the new process's stack into our swap space */
	vm_set_swap_stack(rproc->pgdir, new_proc->pgdir);
	struct trap_frame* tf = (struct trap_frame*)
		((char*)new_proc->tf - SVM_DISTANCE);
	tf->eax = 0; /* The child should return 0 */
	/* new proc needs a context */
	struct context* c = (struct context*)
		((char*)tf - sizeof(struct context));
	/* Set the return to fork_return */
	c->eip = (uintptr_t)fork_return;
	c->esp = (uintptr_t)tf - 4 + SVM_DISTANCE;
	c->cr0 = (uintptr_t)new_proc->pgdir;
	new_proc->context = (uintptr_t)c + SVM_DISTANCE;

	/* Clear the swap stack now */
	vm_clear_swap_stack(rproc->pgdir);

	if(flags & CLONE_VFORK)
	{
		/* Copy the file table over */
		new_proc->fdtab = main_proc->fdtab;
		new_proc->fdtab_lock = main_proc->fdtab_lock;

#ifdef _ALLOW_VM_SHARE_
		/* Map the page directory */
		new_proc->pgdir = main_proc->pgdir;
#endif

		/* Allow the child to run */
		new_proc->state = PROC_RUNNABLE;

#ifdef _ALLOW_VM_SHARE_
		/* Wait for the child to exit */
		while(waitpid_nolock_noharvest(new_proc->pid) 
				!= new_proc->pid);
#else
		while(waitpid_nolock(new_proc->pid, NULL, 0) 
				!= new_proc->pid);
#endif

		/* Harvest the child */
		memset(new_proc, 0, sizeof(struct proc));
		new_proc->state = PROC_UNUSED;

		/* Parent is allowed to return now */
		slock_release(&ptable_lock);
		return new_proc->tid;
	}

	/* Should we use our parent's fd table? */
	if(flags & CLONE_FILES)
	{
		new_proc->fdtab = main_proc->fdtab;
		new_proc->fdtab_lock = main_proc->fdtab_lock;
	}


	slock_release(&ptable_lock);
	return -1;
}

/* int waitpid(int pid, int* status, int options) */
int sys_waitpid(void)
{
	int pid;
	int* status;
	int options = 0;
	if(syscall_get_int(&pid, 0)) return -1;
	if(syscall_get_int((int*)&status, 1)) return -1;
	if(status != NULL && syscall_get_buffer_ptr(
				(void**)&status, sizeof(int), 1))
		return -1;
	if(syscall_get_int(&options, 2)) return -1;

#ifdef DEBUG
	cprintf("%s:%d: Waiting for child %d.\n",
		rproc->name, rproc->pid, pid);
#endif

	return waitpid(pid, status, options);
}

int waitpid(int pid, int* status, int options)
{
	slock_acquire(&ptable_lock);

	int result = waitpid_nolock(pid, status, options);
	/* Release the ptable lock */
	slock_release(&ptable_lock);

	return result;
}

int waitpid_nolock(int pid, int* status, int options)
{
	int ret_pid = 0;
	struct proc* p = NULL;
	while(1)
	{
		int found = 0;
		int process;
		for(process = 0;process < PTABLE_SIZE;process++)
		{
			/* Did we find the process? */
			if(pid == ptable[process].pid)
				found = 1;

			if(ptable[process].state == PROC_ZOMBIE
					&& ptable[process].parent == rproc)
			{
				if(pid == -1 || ptable[process].pid == pid)
				{
					p = ptable + process;
					break;
				}
			}
		}

		/* If no process exists, this process will wait forever */
		if(!found)
		{
			return -1;
		}

		if(p)
		{
			/* Harvest the child */
			ret_pid = p->pid;
			if(status)
				*status = p->return_code;
			/* Free used memory */
			freepgdir(p->pgdir);

			/* pushcli here */
			/* change rproc to the child process so that we can close its files. */
			struct proc* current = rproc;
			rproc = p;
			/* Close open files */
			int file;
			for(file = 0;file < PROC_MAX_FDS;file++)
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

	return ret_pid;
}

int waitpid_nolock_noharvest(int pid)
{
	int ret_pid = -1;
	struct proc* p = NULL;
	while(1)
	{
		int process;
		for(process = 0;process < PTABLE_SIZE;process++)
		{
			if(ptable[process].state == PROC_ZOMBIE
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
			ret_pid = p->pid;
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

	return ret_pid;	
}

/* int wait(int* status) */
int sys_wait(void)
{
	int* status;
	if(syscall_get_int((int*)&status, 0)) return -1;
	if(status != NULL && syscall_get_buffer_ptr(
				(void**)&status, sizeof(int), 0))
		return -1;
	return waitpid(-1, status, 0);
}


/* int execvp(const char* path, const char* args[]) */
int sys_execvp(void)
{
	//return execve(path, argv, NULL);
	cprintf("chronos: execvp not implemented!\n");
	return -1;
}

/* int execl(const char* path, const char* arg0, ...) */
int sys_execl(void)
{
	const char* path;
	char* argv[MAX_ARG];

	syscall_get_str_ptr(&path, 0);
	int i;
	for(i = 0;i < MAX_ARG;i++)
	{
		syscall_get_str_ptr((const char**)(argv + i), i + 1);
		if(!argv[i]) break;
	}

	argv[i] = (char*)NULL;

	return execve(path, argv, NULL);
}

/* int execve(const char* path, char* const argv[], char* const envp[]) */
int sys_execve(void)
{
	const char* path;
	const char** argv;
	const char** envp;
	/* envp is allowed to be null */

	if(syscall_get_str_ptr(&path, 0)) return -1;;
	if(syscall_get_buffer_ptrs((void***)&argv, 1)) return -1;
	if(syscall_get_optional_ptr((void**)&envp, 2))
		return -1;

	return execve(path, (char* const*)argv, (char* const*)envp);
}

int execve(const char* path, char* const argv[], char* const envp[])
{
#ifdef DEBUG
	cprintf("%s:%d: executing program %s\n", 
		rproc->name, rproc->pid, path);

	cprintf("Arguments: \n");
	int t;
	for(t = 0;t < 50;t++)
	{
		if(!argv[t]) break;
		cprintf("\t%d: %s\n", t, argv[t]);
	}

	if(envp)
	{
		cprintf("Environment:\n");
		for(t = 0;t < 50;t++)
		{
			if(!envp[t]) break;
			cprintf("\t%d: %s\n", t, envp[t]);
		}
		cprintf("Program CWD: %s\n", rproc->cwd);
	} else {
		cprintf("ERROR: environment pointer is bad.\n");
	}
#endif

	/* Create a copy of the path */
	char program_path[MAX_PATH_LEN];
	memset(program_path, 0, MAX_PATH_LEN);
	strncpy(program_path, path, MAX_PATH_LEN);

	char cwd_tmp[MAX_PATH_LEN];
	memmove(cwd_tmp, rproc->cwd, MAX_PATH_LEN);

	vmflags_t dir_flags = VM_DIR_WRIT | VM_DIR_READ | VM_DIR_USRP;
	vmflags_t tbl_flags = VM_TBL_WRIT | VM_TBL_READ | VM_TBL_USRP;

	/* Does the binary look ok? */
	if(elf_check_binary_path(path))
	{
		/* see if it is available in /bin */
		strncpy(rproc->cwd, "/bin/", MAX_PATH_LEN);
		if(elf_check_binary_path(path))
		{
			/* It really doesn't exist. */
			memmove(rproc->cwd, cwd_tmp, MAX_PATH_LEN);

#ifdef DEBUG
			cprintf("%s:%d: Binary not found! %s\n", 
					rproc->name, rproc->pid, path);
#endif
			return -1;
		}
	}

	/* acquire ptable lock */
	slock_acquire(&ptable_lock);

	/* Create a temporary address space */
	pgdir_t* tmp_pgdir = (pgdir_t*)palloc();

	uint env_start = PGROUNDUP(UVM_TOP);
	int null_buff = 0x0;
	if(envp)
	{
		/* Get the size of the  environment space */
		int env_sz = 0;
		int index;
		for(index = 0;envp[index];index++)
			env_sz += strlen(envp[index]) + 1;
		/* take in account the array */
		int env_count = index;
		/* NOTE: remember that envp is null terminated */
		env_sz += (env_count * sizeof(int)) + sizeof(int);
		/* Get the start of the environment space */
		env_start = PGROUNDDOWN(UVM_TOP - env_sz);
		uint* env_arr = (uint*)env_start;
		uchar* env_data = (uchar*)env_start + 
			(env_count * sizeof(int)) + sizeof(int);
		for(index = 0;index < env_count;index++)
		{
			/* Set the value in env_arr */
			vm_memmove(env_arr + index, &env_data, sizeof(int),
					tmp_pgdir, rproc->pgdir,
					dir_flags, tbl_flags);

			/* Move the string */
			int len = strlen(envp[index]) + 1;
			vm_memmove(env_data, envp[index], len, 
					tmp_pgdir, rproc->pgdir,
					dir_flags, tbl_flags);
			env_data += len;
		}
		/* Add null element */
		vm_memmove(env_arr + index, &null_buff, sizeof(int),
				tmp_pgdir, rproc->pgdir,
				dir_flags, tbl_flags);
	} else {
		env_start -= sizeof(int);
		vm_memmove((void*)env_start, &null_buff, sizeof(int),
				tmp_pgdir, rproc->pgdir,
				dir_flags, tbl_flags);
	}

	/* Create argument array */
	char* args[MAX_ARG];
	memset(args, 0, MAX_ARG * sizeof(char*));

	uint uvm_stack = PGROUNDDOWN(env_start); 
	uint uvm_start = uvm_stack;
	/* copy arguments */
	int x;
	for(x = 0;argv[x];x++)
	{
		uint str_len = strlen(argv[x]) + 1;
		uvm_stack -= str_len;
		vm_memmove((void*)uvm_stack, (char*)argv[x], str_len,
				tmp_pgdir, rproc->pgdir,
				dir_flags, tbl_flags);
		args[x] = (char*)uvm_stack;
	}
	/* Copy argument array */
	uvm_stack -= MAX_ARG * sizeof(char*);
	/* Integer align the arr (performance) */
	uvm_stack &= ~(sizeof(int) - 1);
	/* Copy the array */
	vm_memmove((void*)uvm_stack, args, MAX_ARG * sizeof(char*),
			tmp_pgdir, rproc->pgdir,
			dir_flags, tbl_flags);

	uint arg_arr_ptr = uvm_stack; /* argv */
	int arg_count = x;

	/* Push envp */
	uvm_stack -= sizeof(int);
	vm_memmove((void*)uvm_stack, &env_start, sizeof(int),
			tmp_pgdir, rproc->pgdir,
			dir_flags, tbl_flags);

	/* Push argv */
	uvm_stack -= sizeof(int);
	vm_memmove((void*)uvm_stack, &arg_arr_ptr, sizeof(int),
			tmp_pgdir, rproc->pgdir,
			dir_flags, tbl_flags);

	/* push argc */
	uvm_stack -= sizeof(int);
	vm_memmove((void*)uvm_stack, &arg_count, sizeof(int),
			tmp_pgdir, rproc->pgdir,
			dir_flags, tbl_flags);

	/* Add bogus return address for _start */
	uvm_stack -= sizeof(int);
	uint ret_addr = 0xFFFFFFFF;
	vm_memmove((void*)uvm_stack, &ret_addr, sizeof(int),
			tmp_pgdir, rproc->pgdir,
			dir_flags, tbl_flags);

	/* Set stack start and stack end */
	rproc->stack_start = PGROUNDUP(uvm_start);
	rproc->stack_end = PGROUNDDOWN(uvm_stack);

	/* Is this a thread? */
	if(rproc->pid == rproc->tgid || 1)
	{
		/* Free user memory */
		vm_free_uvm(rproc->pgdir);
	} else {
		cprintf("Thread called exec!\n");
		/* Create new page directory */
		pgdir_t* dir = (pgdir_t*)palloc();
		/* Copy over the kvm */
		vm_copy_kvm(dir);
		/* Copy over the current kstack */
		vm_set_user_kstack(dir, rproc->pgdir);
		/* Assign the new page directory */
		rproc->pgdir = dir;
		/* Clear the TLB */
		// switch_uvm(rproc);
	}

	/* Map user pages */
	vm_map_uvm(rproc->pgdir, tmp_pgdir);

	/* Invalidate the TLB */
	switch_uvm(rproc);

	/* load the binary if possible. */
	uintptr_t code_start;
	uintptr_t code_end;
	uintptr_t entry = elf_load_binary_path(program_path, rproc->pgdir,
			&code_start, &code_end, 1);

	if(entry == 0)
	{
		memmove(rproc->cwd, cwd_tmp, MAX_PATH_LEN);
		/* Free temporary directory */
		freepgdir(tmp_pgdir);
		slock_release(&ptable_lock);
		return -1;
	}

	rproc->code_start = code_start;
	rproc->code_end = code_end;
	rproc->entry_point = entry;

	/* Change name */
	strncpy(rproc->name, program_path, FILE_MAX_PATH);

	/* We now have the esp and ebp. */
	rproc->tf->esp = uvm_stack;
	rproc->tf->ebp = rproc->tf->esp;

	/* Set eip to correct entry point */
	rproc->tf->eip = rproc->entry_point;

	/* Adjust heap start and end */
	rproc->heap_start = PGROUNDUP(code_end);
	rproc->heap_end = rproc->heap_start;

#ifdef DEBUG
	cprintf("Code Segment:\n");
	cprintf("\tBinary size: %d KB\n", (code_end - code_start) >> 10);
	cprintf("\tCode Boundaries: 0x%x -> 0x%x\n", code_start, code_end);
	cprintf("\tStart of heap: 0x%x\n", rproc->heap_start);
#endif

	uchar setuid = 0;
	uchar setgid = 0;
	/* Check for setuid and setgid */
	inode i = fs_open(program_path, O_RDONLY, 0x0, 0x0, 0x0);

	if(!i) 
	{
		cprintf("exec: file was deleted while reading.\n");
		slock_release(&ptable_lock);
		return -1;
	}

	struct stat st;
	if(fs_stat(i, &st)) 
	{
		slock_release(&ptable_lock);
		return -1;
	}
	if(st.st_mode & S_ISUID)
		setuid = 1;
	if(st.st_mode & S_ISGID)
		setuid = 1;
	fs_close(i);

	/* change permission if needed */
	if(setuid)
		rproc->euid = rproc->uid = st.st_uid;
	if(setgid)
		rproc->egid = rproc->gid = st.st_gid;

	/* restore cwd */
	memmove(rproc->cwd, cwd_tmp, MAX_PATH_LEN);

	/* Free temporary space */
	vm_freepgdir_struct(tmp_pgdir);

	/* Reset all ticks */
	rproc->user_ticks = 0;
	rproc->kernel_ticks = 0;

	/* unset signal stack info */
	rproc->sig_stack_start = 0;
	sig_clear(rproc);

	/* Set mmap area start */
	rproc->mmap_end = rproc->mmap_start = 
		PGROUNDDOWN(uvm_stack) - UVM_MIN_STACK;

	/* Release the ptable lock */
	slock_release(&ptable_lock);

#ifdef DEBUG
	cprintf("%s:%d: Binary load success.\n",
			rproc->name, rproc->pid);
#endif

	return 0;
}

int sys_getpid(void)
{
	return rproc->pid;
}

int sys_exit(void)
{
#ifdef DEBUG
	cprintf("\n\n%s:%d $$$exiting$$$ -- standard exit call\n\n",
			rproc->name, rproc->pid);
#endif

	/* Acquire the ptable lock */
	slock_acquire(&ptable_lock);

	int return_code = 1;
	if(syscall_get_int(&return_code, 0)) ; /* Exit cannot fail */
	rproc->return_code = return_code;

	/* Set state to zombie */
	rproc->state = PROC_ZOMBIE;

	/* Attempt to wakeup our parent */
	if(rproc->orphan == 0)
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

void _exit(int return_code)
{
	rproc->sys_esp = (uint*)&return_code;	
	sys_exit(); /* Will not return */
	panic("_exit returned.\n");
}

int sys__exit(void)
{
	slock_acquire(&ptable_lock);
	/* Set state to killed */
	int return_code;
	if(syscall_get_int(&return_code, 0)) return -1;
	rproc->return_code = return_code;
	rproc->state = PROC_ZOMBIE;
	/* Clear all of the file descriptors (LEAK) */
	// int x;
	// for(x = 0;x < MAX_FILES;x++)
	//	rproc->file_descriptors[x].type = 0x0;

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
	uintptr_t check_addr = (uintptr_t)addr;
	if(PGROUNDUP(check_addr) >= rproc->stack_end
			|| check_addr < rproc->heap_start)
	{
		slock_release(&ptable_lock);
		return -1;
	}

	/* Change the address break */
	uintptr_t old = rproc->heap_end;

	/* Check to make sure the address is okay */
	if((uintptr_t)addr < rproc->code_end || 
			(uintptr_t)addr > rproc->stack_end)
	{
#ifdef DEBUG
		cprintf("%s:%d: ERROR: program gave bad brk addr: 0x%x\n",
				rproc->name, rproc->pid, addr);
#endif

		slock_release(&ptable_lock);
		return old;
	}

	rproc->heap_end = (uintptr_t)addr;

#ifdef DEBUG
	cprintf("%s:%d: Old program break: 0x%x\n", 
			rproc->name, rproc->pid, rproc->heap_end);
#endif

	/* Unmap pages */
	old = PGROUNDUP(old);
	vmpage_t page = PGROUNDUP(rproc->heap_end);
	for(;page != old;page += PGSIZE)
	{
		/* Free the page */
		uintptr_t pg = vm_unmappage(page, rproc->pgdir);
		if(pg) 
		{
			pfree(pg);
		}
	}

#ifdef DEBUG
	cprintf("%s:%d: New program break: 0x%x\n", 
			rproc->name, rproc->pid, rproc->heap_end);
#endif

	/* Release lock */
	slock_release(&ptable_lock);
	return (int)addr;
}

/* void* sys_sbrk(uint increment) */
int sys_sbrk(void)
{
	intptr_t increment;
	if(syscall_get_int((int*)&increment, 0)) return -1;

	/* See if they are just checking for the program break */
	if(increment == 0)
	{
#ifdef DEBUG
		cprintf("%s:%d: checked program break: 0x%x\n",
				rproc->name, rproc->pid, rproc->heap_end);
#endif
		return rproc->heap_end;
	}

	slock_acquire(&ptable_lock);
	uintptr_t old_end = rproc->heap_end;

	if(increment < 0)
	{
		/* Deallocating pages */
		uintptr_t new_page = PGROUNDDOWN(old_end + increment - 1);
		uintptr_t old_page = PGROUNDDOWN(old_end - 1);

		for(;old_page > new_page;old_page -= PGSIZE)
		{
			if(!vm_unmappage(old_page, rproc->pgdir))
				panic("%s: Couldn't unmap page: 0x%x\n",
						rproc->name,
						old_page);
		}

	} else {
		/* Will this collide with the stack? */
		if(PGROUNDUP(old_end + increment) >= rproc->stack_end
				|| (rproc->mmap_end &&
					PGROUNDUP(old_end + increment) 
					>= rproc->mmap_end))
		{
			slock_release(&ptable_lock);
			return (int)NULL;
		}

		/* Do we even need to map pages? */
		if(PGROUNDUP(old_end) != PGROUNDUP(old_end + increment))
		{
			/* Map needed pages */
			vm_mappages(old_end, increment, rproc->pgdir, 
					VM_DIR_USRP | VM_DIR_READ 
					| VM_DIR_WRIT,
					VM_TBL_USRP | VM_TBL_READ 
					| VM_TBL_WRIT);
		}
		/* Zero space (security) */
		memset((void*)old_end, 0, increment);
	}

#ifdef DEBUG
	cprintf("%s:%d: Program used sbrk    0x%x --> 0x%x\n",
			rproc->name, rproc->pid, old_end,
			old_end + increment);
	cprintf("%s:%d:\t\tHeap size change: %d KB\n", 
			rproc->name, rproc->pid, (increment >> 12));
#endif

	/* Change heap end */
	rproc->heap_end = old_end + increment;
	/* release lock */
	slock_release(&ptable_lock);

	/* return old end */
	return (int)old_end;
}

/* int chdir(const char* dir) */
int sys_chdir(void)
{
	const char* dir;
	if(syscall_get_str_ptr(&dir, 0)) return -1;

	if(strlen(dir) < 1) return -1;
	char dir_path[MAX_PATH_LEN];
	char tmp_path[MAX_PATH_LEN];
	strncpy(dir_path, dir, MAX_PATH_LEN);
	if(file_path_dir(dir_path, MAX_PATH_LEN))
		return -1;
	if(fs_path_resolve(dir_path, tmp_path, MAX_PATH_LEN))
		return -1;
	if(strlen(tmp_path) < 1) return -1;
	strncpy(dir_path, tmp_path, MAX_PATH_LEN);
	if(file_path_file(dir_path))
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

	/* Make it a directory now */
	if(file_path_dir(dir_path, MAX_PATH_LEN))
		return -1;

	/* Everything is ok. */
	memmove(rproc->cwd, dir_path, MAX_PATH_LEN);
	return 0;
}

/* int getcwd(char* dst, uint sz) */
int sys_getcwd(void)
{
	char* dst;
	uint sz;	

	if(syscall_get_int((int*)&sz, 1)) return -1;
	if(syscall_get_buffer_ptr((void**)&dst, sz, 0)) return -1;
	strncpy(dst, rproc->cwd, sz);
	return (int)dst;
}

/* clock_t times(struct tms* buf) */
int sys_times(void)
{
	struct tms* buf;
	/* buf is allowed to be null */
	if(syscall_get_int((int*)&buf, 0)) return -1;
	if(buf && syscall_get_buffer_ptr((void**)&buf, sizeof(struct tms), 0))
		return -1;

	if(buf)
	{
		buf->tms_utime = rproc->user_ticks;
		buf->tms_stime = rproc->kernel_ticks;
		/* Update with child information */
		buf->tms_cutime = 0;
		buf->tms_cstime = 0;
	}

	return k_ticks;
}

/* int gettimeofday(struct timeval* tv, struct timezone* tz) */
int sys_gettimeofday(void)
{
	struct timeval* tv;
	struct timezone* tz;
	/* timezone is not used by any OS ever. It is purely historical. */
	if(syscall_get_int((int*)&tv, 0)) return -1;
	if(tv && syscall_get_buffer_ptr((void**)&tv, 
				sizeof(struct timeval), 0)) return -1;
	/* Is timezone specified? */
	if(syscall_get_int((int*)&tz, 1)) return -1;
	if(tz && syscall_get_buffer_ptr((void**)&tz, 
				sizeof(struct timeval), 1)) return -1;

	int seconds = ktime_seconds();

	if(tv)
	{
		tv->tv_sec = seconds;
		tv->tv_usec = 0;
	}

	if(tz)
	{
		memset(tz, 0, sizeof(struct timezone));
	}

	return 0;
}

/* uid_t getuid(void) */
int sys_getuid(void){
	return rproc->uid;
}

/* uid_t getuid(void) */
int sys_geteuid(void){
	return rproc->euid;
}

/* int setuid(uid_t uid)*/
int sys_setuid(void){
	uint id;

	if(syscall_get_int((int*)&id, 0)) return -1;
	rproc->uid = id;
	return 0;
}

/* gid_t getgid(void) */
int sys_getgid(void){
	return rproc->gid;
}

/* gid_t getegid(void) */
int sys_getegid(void){
	return rproc->egid;
}

/* int setegid(gid_t gid) */
int sys_setegid(void)
{
	gid_t g;
	if(syscall_get_int((int*)&g, 0)) return -1;
	rproc->egid = g;
	return 0;
}

/* int setgid(gid_t gid)*/
int sys_setgid(void){
	uint id;

	if(syscall_get_int((int*)&id, 0)) return -1;
	rproc->gid = id;
	return 0;
}

/* pid_t gettid(void) */
int sys_gettid(void){
	return rproc->tid;
}

/* pid_t getppid(void) */
int sys_getppid(void){
	if(!rproc->parent) return -1;
	return (rproc->parent)->pid;
}

/* int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid) */
int sys_getresuid(void){
	uid_t *ruid;
	uid_t *euid;
	uid_t *suid;
	if(syscall_get_buffer_ptr((void**)&ruid, sizeof(int*), 0)) return -1;
	if(syscall_get_buffer_ptr((void**)&euid, sizeof(int*), 1)) return -1;
	if(syscall_get_buffer_ptr((void**)&suid, sizeof(int*), 2)) return -1;	
	*ruid = rproc->ruid;
	*euid = rproc->euid;
	*suid = rproc->suid;
	return 0;

}

/* int getresuid(gid_t *rgid, gid_t *egid, gid_t *sgid) */
int sys_getresgid(void){
	gid_t *rgid;
	gid_t *egid;
	gid_t *sgid;
	if(syscall_get_buffer_ptr((void**)&rgid, sizeof(int*), 0))return -1;
	if(syscall_get_buffer_ptr((void**)&egid, sizeof(int*), 1))return -1;
	if(syscall_get_buffer_ptr((void**)&sgid, sizeof(int*), 2)) return -1;
	*rgid = rproc->rgid;
	*egid = rproc->egid;
	*sgid = rproc->sgid;
	return 0;	
}

/* pid_t getsid(pid_t pid) */
int sys_getsid(void){
	pid_t pid;
	syscall_get_int((int*)&pid, 0);

	if(pid == 0){
		return rproc->sid;
	} else {
		struct proc* p = get_proc_pid(pid);
		if(p) return p->sid;
	}

	return -1;
}

int sys_setsid(void){
	if(rproc->sid == rproc->pid)
		return -1; /* Already session leader */
	rproc->sid = rproc->pid;
	rproc->pgid = rproc->pid;
	return rproc->sid;
}

/* int setpgid(pid_t pid, pid_t pgid); */
int sys_setpgid(void)
{
	pid_t pid;
	pid_t pgid;

	if(syscall_get_int(&pid, 0)) return -1;
	if(syscall_get_int(&pgid, 1)) return -1;

	struct proc* p = get_proc_pid(pid);
	if(!p) return -1;
	p->pgid = pgid;

	return 0;
}

/* pid_t getpgid(pid_t pid) */
int sys_getpgid(void){
	pid_t pid;
	if(syscall_get_int((int*)&pid, 0)) return -1;
	if(pid == 0){
		return rproc->pgid;
	} else {
		struct proc* p = get_proc_pid(pid);
		if(p) return p->pgid;
	}

	return -1;	
}

/* pid_t getpgrp(void) */
int sys_getpgrp(void)
{
	return rproc->pgid; 
}

/* int setresuid(uid_t ruid, uid_t euid, uid_t suid) */
int sys_setresuid(void){
	uid_t ruid;
	uid_t euid;
	uid_t suid;
	if(syscall_get_int((int*)&ruid, 0)) return -1;
	if(syscall_get_int((int*)&euid, 1)) return -1;
	if(syscall_get_int((int*)&suid, 2)) return -1;

	uid_t arr[3];
	arr[0] = ruid;
	arr[1] = euid;
	arr[2] = suid; 	
	int i;
	if(rproc->uid!=0){ //not privileged
		for(i = 0; i<3; i++){
			if(arr[i]!=rproc->ruid && arr[i]!=rproc->euid && arr[i]!=rproc->suid && arr[i]!=-1){
				cprintf("sysproc.c setresuid: invalid uid for unprivileged processes \n");
				return -1;
			}
		}
		if(ruid != -1){
			rproc->ruid = ruid;
		}
		if(euid != -1){
			rproc->euid = euid;
		}
		if(suid != -1){
			rproc->suid = suid;
		}
	}
	else{ /* privileged process*/
		if(ruid != -1){
			rproc->ruid = ruid;
		}
		if(euid != -1){
			rproc->euid = euid;	
		}
		if(suid != -1){
			rproc->suid = suid;
		}
	}
	return 0;

}
/* int setresgid(gid_t rgid, gid_t egid, gid_t sgid)*/
int sys_setresgid(void){
	gid_t rgid;
	gid_t egid;
	gid_t sgid;
	if(syscall_get_int((int*)&rgid, 0)) return -1;
	if(syscall_get_int((int*)&egid, 1)) return -1;
	if(syscall_get_int((int*)&sgid, 2)) return -1;
	gid_t arr[3];
	arr[0] = rgid;
	arr[1] = egid;
	arr[2] = sgid; 	
	int i;
	if(rproc->gid!=0){ //not privileged
		for(i = 0; i<3; i++){
			if(arr[i]!=rproc->rgid && arr[i]!=rproc->egid && arr[i]!=rproc->sgid && arr[i]!=-1){
				cprintf("sysproc.c setresgid: invalid gid for unprivileged processes \n");
				return -1;
			}
		}
		if(rgid != -1){
			rproc->rgid = rgid;
		}
		if(egid != -1){
			rproc->egid = egid;
		}
		if(sgid != -1){
			rproc->sgid = sgid;
		}
	}
	else{ /* privileged process*/
		if(rgid != -1){
			rproc->rgid = rgid;
		}
		if(egid != -1){
			rproc->egid = egid;	
		}
		if(sgid != -1){
			rproc->sgid = sgid;
		}
	}
	return 0;

}
/* int setreuid(uid_t ruid, uid_t euid)*/
int sys_setreuid(void){
	uid_t ruid;
	uid_t euid;
	if(syscall_get_int((int*)&ruid, 0)) return -1;
	if(syscall_get_int((int*)&euid, 1)) return -1;	
	uid_t old_ruid = rproc->ruid;
	if(rproc->uid!=0){ //not privileged
		if(euid!=rproc->ruid && euid!=rproc->euid && euid!=rproc->suid && euid!=-1){
			cprintf("sysproc.c setreuid: invalid uid for unprivileged processes \n");
			return -1;
		}
		if(ruid!=rproc->ruid && ruid!=rproc->euid && ruid!=-1){
			cprintf("sysproc.c setreuid: invalid uid for unprivileged processes \n");
			return -1;
		}
		if(euid!=-1){
			rproc->euid = euid;
		}
		if(ruid!=-1){
			rproc->ruid = ruid;
		}
		if((ruid!=-1 || euid!=old_ruid) && euid!= -1 ){
			rproc->suid = euid;
		}
	}
	else{ /* privileged process*/
		if(ruid != -1){
			rproc->ruid = ruid;
		}
		if(euid != -1){
			rproc->euid = euid;	

		}
		if((ruid!=-1 || euid!=old_ruid) && euid!= -1 ){
			rproc->suid = euid;
		}
	}
	return 0;
}

/*int setregid(gid_t rgid, gid_t egid);*/
int sys_setregid(void){
	uid_t rgid;
	uid_t egid;
	if(syscall_get_int((int*)&rgid, 0)) return -1;
	if(syscall_get_int((int*)&egid, 1)) return -1;	
	uid_t old_rgid = rproc->rgid;
	if(rproc->gid!=0){ //not privileged
		if(egid!=rproc->rgid && egid!=rproc->egid && egid!=rproc->sgid && egid!=-1){
			cprintf("sysproc.c setregid: invalid uid for unprivileged processes \n");
		}
		if(rgid!=rproc->rgid && rgid!=rproc->egid && rgid!=-1){
			cprintf("sysproc.c setrguid: invalid uid for unprivileged processes \n");
		}
		if(egid!=-1){
			rproc->egid = egid;
		}
		if(rgid!=-1){
			rproc->rgid = rgid;
		}
		if((rgid!=-1 || egid!=old_rgid) && egid!= -1 ){
			rproc->sgid = egid;
		}
	}
	else{ /* privileged process*/
		if(rgid != -1){
			rproc->rgid = rgid;
		}
		if(egid != -1){
			rproc->egid = egid;	

		}

		if((rgid != -1 || egid != old_rgid) && egid!= -1 ){
			rproc->sgid = egid;
		}
	}
	return 0;
}

/* mode_t umask(mode_t mask) */
int sys_umask(void){
	mode_t mask;
	if(syscall_get_int((int*)&mask, 0)) return -1;
	rproc->umask = (mask & 0777);
	return mask;
}

int getumask(void){
	return rproc->umask;
}

/* uint sleep(uint seconds) */
int sys_sleep(void)
{
	uint seconds;
	if(syscall_get_int((int*)&seconds, 0)) return -1;
	slock_acquire(&ptable_lock);
	rproc->block_type = PROC_BLOCKED_SLEEP;
	int end = seconds + ktime_seconds() + 1;
	rproc->sleep_time = end;
	rproc->state = PROC_BLOCKED;
	while(1)
	{
		yield_withlock();
		if(ktime_seconds() >= end) break;
		slock_acquire(&ptable_lock);
	}

	return 0;
}

int sys_alarm(void)
{
	panic("WARNING: alarm system call unimplemented.\n");
	return -1;
}

int sys_vfork(void)
{
#ifdef _ALLOW_VM_SHARE_
	return clone(CLONE_VFORK, NULL, NULL, NULL, NULL);
#else
	/* Create a new child */
	pid_t p = sys_fork();

	if(p > 0)
	{
		slock_acquire(&ptable_lock);
		if(waitpid_nolock_noharvest(p) != p)
		{
#ifdef DEBUG
			cprintf("chronos: vfork failed! 2\n");	
			slock_release(&ptable_lock);
			return -1;
#endif
		}
		slock_release(&ptable_lock);
	} else {
#ifdef DEBUG
		cprintf("chronos: vfork failed!\n");
		return -1;
#endif
	}

	return p;
#endif
}
