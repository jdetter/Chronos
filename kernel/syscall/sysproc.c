#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/wait.h>

#include "kstdlib.h"
#include "syscall.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "chronos.h"
#include "vm.h"
#include "trap.h"
#include "panic.h"
#include "signal.h"
#include "elf.h"
#include "sched.h"

// #define DEBUG

#ifdef RELEASE
# undef DEBUG
#endif

extern int k_ticks;

int waitpid_nolock(int pid, int* status, int options);
int waitpid_nolock_noharvest(int pid);

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

	int result = waitpid_nolock(pid, status, options);
	/* Release the ptable lock */
	slock_release(&ptable_lock);

	return result;
}

int waitpid_nolock(int pid, int* status, int options)
{
#ifdef DEBUG
	cprintf("%s:%d: Waiting for pid %d\n", rproc->name, rproc->pid,
		pid);
#endif

	int ret_pid = 0;
	struct proc* p = NULL;
	while(1)
	{
		int found = 0; /* Is there an elligible process? */
		int process;
		for(process = 0;process < PTABLE_SIZE;process++)
		{
			/* Did we find the process? */
			if((pid == ptable[process].pid || pid == -1)
				&& ptable[process].parent == rproc)
				found = 1;
			else continue;

			int status_change = ptable[process].status_changed;
#ifdef DEBUG
			cprintf("%s:%d: Child changed status? %d\n",
					rproc->name, rproc->pid, status_change);
#endif

			/* Are we listening to continue events? */
			if((ptable[process].state == PROC_RUNNABLE 
					|| ptable[process].state == PROC_RUNNING)
					&& ((options & WCONTINUED) == 0x0))
				status_change = 0;

#ifdef DEBUG
			cprintf("%s:%d: Listening to continues? %d\n",
					rproc->name, rproc->pid, status_change);
#endif

			/* Are we listening to stops? */
			if(ptable[process].state == PROC_STOPPED
					&& (options & WUNTRACED) == 0x0)
				status_change = 0;

#ifdef DEBUG
			cprintf("%s:%d: Waiting for stops? %d\n",
					rproc->name, rproc->pid, status_change);
#endif

			if((ptable[process].state == PROC_ZOMBIE || status_change)
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
#ifdef DEBUG
			cprintf("%s:%d: NO SUCH PROCESS FOUND.\n",
				rproc->name, rproc->pid);
#endif
			return -1;
		}

		if(p)
		{
			ret_pid = p->pid;
			if(p->state == PROC_ZOMBIE)
			{
				/* Harvest the child */
				if(status)
					*status = p->return_code;

				/* Free used memory */
				freepgdir(p->pgdir);

				/* pushcli here */

				/**
				 * Change rproc to the child process so that we 
				 * can close its files. 
				 */
				struct proc* current = rproc;
				rproc = p;

				/* Close open files */
				int file;
				for(file = 0;file < PROC_MAX_FDS;file++)
					close(file);
				rproc = current;

				memset(p, 0, sizeof(struct proc));
				p->state = PROC_UNUSED;
			} else { /* The process wasn't ended */
				p->status_changed = 0;

				switch(rproc->state)
				{
					/* Continued */
					case PROC_RUNNABLE:
					case PROC_RUNNING:
						*status = 0x05;
						break;
					/* Stopped */
					case PROC_STOPPED:
						*status = 0x04;
						break;
					default:
						panic("kernel: tried to wakeup on "
								"process we don't care about");
						break;
				}
			}

			break;
		} else {
			/* Set the wait options */
			rproc->wait_options = options;

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

#ifdef DEBUG
	cprintf("%s:%d: wokeup on process with pid %d\n",
			rproc->name, rproc->pid, ret_pid);
#endif

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

int sys_getpid(void)
{
	return rproc->pid;
}

int sys_exit(void)
{
	int return_code = 1;
	if(syscall_get_int(&return_code, 0)) ; /* Exit cannot fail */
#ifdef DEBUG
	cprintf("\n\n%s:%d exiting -- status: %d\n\n",
			rproc->name, rproc->pid, return_code);
#endif

	/* Acquire the ptable lock */
	slock_acquire(&ptable_lock);

	/* The process exited */
	rproc->return_code = (return_code & 0xFF) << 8;

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

void wake_parent(struct proc* p)
{
	/* Attempt to wakeup our parent */
	if(p->orphan == 0)
	{
		if(p->parent->block_type == PROC_BLOCKED_WAIT)
		{
			if(p->parent->b_pid == -1 || p->parent->b_pid == p->pid)
			{
				/* Our parent is waiting on us, release the block. */
				p->parent->block_type = PROC_BLOCKED_NONE;
				p->parent->state = PROC_RUNNABLE;
				p->parent->b_pid = 0;
			}
		}
	}
}


void _exit(int return_code)
{
	rproc->sys_esp = (int*)&return_code;	
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

/* void* sys_sbrk(int increment) */
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

/* int getcwd(char* dst, size_t sz) */
int sys_getcwd(void)
{
	char* dst;
	size_t sz;	

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
	uid_t id;

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
	gid_t id;

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

int sys_alarm(void)
{
	panic("WARNING: alarm system call unimplemented.\n");
	return -1;
}

int sys_vfork(void)
{
#ifdef __ALLOW_VM_SHARE__
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
