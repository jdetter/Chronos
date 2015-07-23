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
#include "rtc.h"
#include "ktime.h"

extern pid_t next_pid;
extern slock_t ptable_lock;
extern uint k_ticks;
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
	new_proc->tid = 0; /* Not a thread */
	new_proc->pgid = rproc->pgid;
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

	return waitpid(pid, status, options);
}

int waitpid(int pid, int* status, int options)
{
	slock_acquire(&ptable_lock);

	int ret_pid = 0;
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
			/* Harvest the child */
			ret_pid = p->pid;
			if(status)
				*status = rproc->return_code;
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

	uchar setuid = 0;
	uchar setgid = 0;
	/* Check for setuid and setgid */
	inode i = fs_open(path, O_RDONLY, 0x0, 0x0, 0x0);
	if(!i) return -1;
	struct stat st;
	if(fs_stat(i, &st)) return -1;
	if(st.st_mode & S_ISUID)
		setuid = 1;
	if(st.st_mode & S_ISGID)
		setuid = 1;
	fs_close(i);

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
	uint uvm_stack = PGROUNDUP(UVM_TOP);
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
	mappage(stack_start, PGROUNDUP(UVM_TOP) - PGSIZE,
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

	/* change permission if needed */
	if(setuid)
		rproc->euid = rproc->uid = st.st_uid;
	if(setgid)
		rproc->egid = rproc->gid = st.st_gid;

	/* Reset all ticks */
	rproc->user_ticks = 0;
	rproc->kernel_ticks = 0;
	
	/* Release the ptable lock */
	slock_release(&ptable_lock);

	return 0;
}

/* int exec(const char* path, char* const argv[], char* const envp[]) */
int sys_execve(void)
{
	const char* path;
	const char** argv;
	const char** envp;
	/* envp is allowed to be null */

	if(syscall_get_str_ptr(&path, 0)) return -1;;
	if(syscall_get_buffer_ptrs((uchar***)&argv, 1)) return -1;
	if(syscall_get_int((int*)&envp, 2)) return -1;
	if(envp && syscall_get_buffer_ptrs((uchar***)&envp, 2)) return -1;

	/* Create a copy of the path */
        char program_path[MAX_PATH_LEN];
        memset(program_path, 0, MAX_PATH_LEN);
        strncpy(program_path, path, MAX_PATH_LEN);

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
	/* acquire ptable lock */
	slock_acquire(&ptable_lock);

	/* Create a temporary address space */
	pgdir* tmp_pgdir = (pgdir*)palloc();

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
		env_sz += (env_count * sizeof(int)) + sizeof(int);
		/* Get the start of the environment space */
		uint env_start = PGROUNDDOWN(UVM_TOP - env_sz);
		uint* env_arr = (uint*)env_start;
		uchar* env_data = (uchar*)env_start + 
			(env_count * sizeof(int)) + sizeof(int);
		for(index = 0;index < env_count;index++)
		{
			/* Set the value in env_arr */
			vm_memmove(env_arr + index, env_data, sizeof(int),
					tmp_pgdir, rproc->pgdir);

			/* Move the string */
			int len = strlen(envp[index]) + 1;
			vm_memmove(env_data, envp[index], len, 
					tmp_pgdir, rproc->pgdir);
			env_data += len;
		}
		vm_memmove(env_arr + index, &null_buff, sizeof(int),
				tmp_pgdir, rproc->pgdir);	
	} else {
		env_start -= sizeof(int);
		vm_memmove((void*)env_start, &null_buff, sizeof(int),
                                tmp_pgdir, rproc->pgdir);
	}

	/* Create argument array */
	char* args[MAX_ARG];
	memset(args, 0, MAX_ARG * sizeof(char*));

	uint uvm_stack = PGROUNDDOWN(env_start); 
	/* copy arguments */
	int x;
	for(x = 0;argv[x];x++)
	{
		uint str_len = strlen(argv[x]) + 1;
		uvm_stack -= str_len;
		vm_memmove((void*)uvm_stack, (char*)argv[x], str_len,
				tmp_pgdir, rproc->pgdir);
		args[x] = (char*)uvm_stack;
	}
	/* Copy argument array */
	uvm_stack -= MAX_ARG * sizeof(char*);
	vm_memmove((void*)uvm_stack, args, MAX_ARG * sizeof(char*),
			tmp_pgdir, rproc->pgdir);

	uint arg_arr_ptr = uvm_stack;
	int arg_count = x;

	/* Push envp */
        uvm_stack -= sizeof(int);
        vm_memmove((void*)uvm_stack, &env_start, sizeof(int),
                        tmp_pgdir, rproc->pgdir);
	
	/* Push argv */
	uvm_stack -= sizeof(int);
	vm_memmove((void*)uvm_stack, &arg_arr_ptr, sizeof(int),
			tmp_pgdir, rproc->pgdir);

	/* push argc */
	uvm_stack -= sizeof(int);
	vm_memmove((void*)uvm_stack, &arg_count, sizeof(int),
			tmp_pgdir, rproc->pgdir);

	/* Add bogus return address (solved by crt0 in stdlibc)*/
	uvm_stack -= sizeof(int);
	uint ret_addr = 0xFFFFFFFF;
	vm_memmove((void*)uvm_stack, &ret_addr, sizeof(int),
			tmp_pgdir, rproc->pgdir);

	/* Free user memory */
	vm_free_uvm(rproc->pgdir);

	/* Map user pages */
	vm_map_uvm(rproc->pgdir, tmp_pgdir);

	/* Invalidate the TLB */
	switch_uvm(rproc);

	/* load the binary if possible. */
	uint load_result = load_binary(program_path, rproc);
	if(load_result == 0)
	{
		memmove(rproc->cwd, cwd_tmp, MAX_PATH_LEN);
		/* Free temporary directory */
		freepgdir(tmp_pgdir);
		slock_release(&ptable_lock);
		return -1;
	}

	/* Change name */
	strncpy(rproc->name, program_path, FILE_MAX_PATH);

	/* We now have the esp and ebp. */
	rproc->tf->esp = uvm_stack;
	rproc->tf->ebp = rproc->tf->esp;

	/* Set eip to correct entry point */
	rproc->tf->eip = rproc->entry_point;

	/* Adjust heap start and end */
	rproc->heap_start = PGROUNDUP(load_result);
	rproc->heap_end = rproc->heap_start;

	uchar setuid = 0;
	uchar setgid = 0;
	/* Check for setuid and setgid */
	inode i = fs_open(program_path, O_RDONLY, 0x0, 0x0, 0x0);
	if(!i) return -1;
	struct stat st;
	if(fs_stat(i, &st)) return -1;
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
	freepgdir_struct(tmp_pgdir);

	 /* Reset all ticks */
        rproc->user_ticks = 0;
        rproc->kernel_ticks = 0;

	/* Release the ptable lock */
	slock_release(&ptable_lock);

	return 0;
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

	int return_code;
	rproc->return_code = 0;
	if(syscall_get_int(&return_code, 0))
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
	sys__exit(); /* Will not return */
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

/* int getcwd(char* dst, uint sz) */
int sys_getcwd(void)
{
	char* dst;
	uint sz;	

	if(syscall_get_int((int*)&sz, 1)) return -1;
	if(syscall_get_buffer_ptr((void**)&dst, sz, 0)) return -1;
	strncpy(dst, rproc->cwd, sz);
	return sz;
}

/* clock_t times(struct tms* buf) */
int sys_times(void)
{
	struct tms* buf;
	/* buf is allowed to be null */
	if(syscall_get_int((int*)buf, 0)) return -1;
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

/* pid_t getuid(void) */
int sys_getuid(void){
	return rproc->uid;
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
int getresuid(void){
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
int getresgid(void){
	gid_t *egid;
	gid_t *sgid;
	gid_t *rgid;
	if(syscall_get_buffer_ptr((void**)&rugid, sizeof(int*), 0))return -1;
	if(syscall_get_buffer_ptr((void**)&eugid, sizeof(int*), 1))return -1;
	if(syscall_get_buffer_ptr((void**)&sugid, sizeof(int*), 2)) return -1;
	*rgid = rproc->rgid;
	*egid = rproc->egid;
	*sgid = rproc->sgid;
	return 0;
	
}

/* pid_t getsid(pid_t pid) */
int getsid(void){
	pid_t pid;
	syscall_get_int((int*)&pid, 0);
	if(pid == 0){
		return rproc->sid;
	}else{
		return (*(get_proc_pid(pid)))->sid;
	}
}

/* int setsid(void) I dont know what I'm doing :)
int setsid(void){
	if(rproc->sid != rproc->pid){
		create new session?
	}
	rproc->sid = rproc->pid;
	rproc->pgid = rproc->pid;
	return rproc->sid;
	return 0;
}*/

/* pid_t getpgid(pid_t pid) */
int getpgid(void){
	pid_t pid;
	if(syscall_get_int((int*)&pid, 0)) return -1;
	if(pid == 0){
		return rproc->pgid;
	}else{
		return (*(get_proc_pid(pid)))->pgid;
	}
	
}

/* pid_t getpgrp(void) */
int getpgrp(void){
	return rproc->pgid; 
}

/* int setresuid(uid_t ruid, uid_t euid, uid_t suid) */
int setresuid(void){
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
	if(rproc->uid!=0){ //not privileged
		for(int i = 0; i<3; i++){
			if(arr[i]!=rproc->ruid && arr[i]!=rproc->euid && arr[i]!=rproc->suid && arr[i]!=-1){
				printf("sysproc.c setresuid: invalid uid for unprivileged processes \n");
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
int setresgid(void){
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
		if(rproc->gid!=0){ //not privileged
			for(int i = 0; i<3; i++){
				if(arr[i]!=rproc->rgid && arr[i]!=rproc->egid && arr[i]!=rproc->sgid && arr[i]!=-1){
					printf("sysproc.c setresgid: invalid gid for unprivileged processes \n");
					return -1;
				}
			}
			if(ruid != -1){
				rproc->rgid = rgid;
			}
			if(euid != -1){
				rproc->egid = egid;
			}
			if(suid != -1){
				rproc->sgid = sgid;
			}
		}
		else{ /* privileged process*/
			if(rgid != -1){
				rproc->rgid = rgid;
			}
			if(euid != -1){
				rproc->egid = egid;	
			}
			if(suid != -1){
				rproc->sgid = sgid;
			}
		}
	return 0;
	
}

/* int setreuid(uid_t ruid, uid_t euid)*/
int setreuid(void){
	uid_t ruid;
	uid_t euid;
	if(syscall_get_int((int*)&ruid, 0)) return -1;
	if(syscall_get_int((int*)&euid, 1)) return -1;	
	uid_t old_ruid = rproc->ruid;
	if(rproc->uid!=0){ //not privileged
		if(euid!=rproc->ruid && euid!=rproc->euid && euid!=rproc->suid && euid!=-1){
			printf("sysproc.c setreuid: invalid uid for unprivileged processes \n");
			return -1;
		}
		if(ruid!=rproc->ruid && ruid!=rproc->euid && ruid!=-1){
			printf("sysproc.c setreuid: invalid uid for unprivileged processes \n");
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
int setregid(void){
	uid_t rgid;
	uid_t egid;
	if(syscall_get_int((int*)&rgid, 0)) return -1;
	if(syscall_get_int((int*)&egid, 1)) return -1;	
	uid_t old_rgid = rproc->rgid;
	if(rproc->gid!=0){ //not privileged
		if(egid!=rproc->rgid && egid!=rproc->egid && egid!=rproc->sgid && egid!=-1){
			printf("sysproc.c setregid: invalid uid for unprivileged processes \n");
		}
		if(rgid!=rproc->rgid && rgid!=rproc->egid && rgid!=-1){
			printf("sysproc.c setrguid: invalid uid for unprivileged processes \n");
		}
		if(egid!=-1){
			rproc->egid = egid;
		}
		if(rgid!=-1){
			rproc->rgid = rgid;
		}
		if((rgid!=-1 || egid!=old_ruid) && egid!= -1 ){
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
		if((rgid!=-1 || euid!=old_ruid) && euid!= -1 ){
			rproc->sgid = egid;
		}
	}
	return 0;
}

int setumask(mode_t mask){
	mode_t mask;
	if(syscall_get_int((int*)&mask, 0)) return -1;
	mode_t prev;
	prev = rproc->umask;
	rproc->umask = (mask & 0777);
	return prev;
}

int getumask(void){
	return rproc->umask;
}



