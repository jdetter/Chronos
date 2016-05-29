#include <sys/select.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/sched.h>

#include "chronos.h"
#include "trap.h"
#include "proc.h"
#include "stdlock.h"
#include "syscall.h"
#include "ktime.h"
#include "x86.h"
#include "drivers/rtc.h"
#include "elf.h"
#include "panic.h"

// #define DEBUG

extern int waitpid_nolock(int pid, int* status, int options);
extern int waitpid_nolock_noharvest(int pid);

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

#ifndef __ALLOW_VM_SHARE__
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
	c->eip = (uintptr_t)tp_fork_return;
	c->esp = (uintptr_t)tf - 4 + SVM_DISTANCE;
	c->cr0 = (uintptr_t)new_proc->pgdir;
	new_proc->context = (uintptr_t)c + SVM_DISTANCE;

	/* Clear the swap stack now */
	vm_clear_swap_stack(rproc->pgdir);

	/* release ptable lock */
	slock_release(&ptable_lock);

	return new_proc->pid;
}

static int clone(unsigned long flags, void* child_stack,
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

#ifndef __ALLOW_VM_SHARE__
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
	c->eip = (uintptr_t)tp_fork_return;
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

#ifdef __ALLOW_VM_SHARE__
		/* Map the page directory */
		new_proc->pgdir = main_proc->pgdir;
#endif

		/* Allow the child to run */
		new_proc->state = PROC_RUNNABLE;

#ifdef __ALLOW_VM_SHARE__
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

/* int clone(unsigned long flags, void* child_stack,
 *      void* ptid, void* ctid,
 *      struct pt_regs* regs) */
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
#ifdef DEBUG
		cprintf("%s:%d: Binary not found! %s\n",
				rproc->name, rproc->pid, path);
#endif
		return -1;
	}

	/* acquire ptable lock */
	slock_acquire(&ptable_lock);

	/* Create a temporary address space */
	pgdir_t* tmp_pgdir = (pgdir_t*)palloc();

	uintptr_t env_start = PGROUNDUP(UVM_TOP);
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
		int* env_arr = (int*)env_start;
		char* env_data = (char*)env_start +
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

	uintptr_t uvm_stack = PGROUNDDOWN(env_start);
	uintptr_t uvm_start = uvm_stack;
	/* copy arguments */
	int x;
	for(x = 0;argv[x];x++)
	{
		int str_len = strlen(argv[x]) + 1;
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

	uintptr_t arg_arr_ptr = uvm_stack; /* argv */
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
	int ret_addr = 0xFFFFFFFF;
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
	vm_enable_paging(rproc->pgdir);

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

	int setuid = 0;
	int setgid = 0;
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
