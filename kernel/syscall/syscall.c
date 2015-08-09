#include "types.h"
#include "x86.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"
#include "devman.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "chronos.h"
#include "stdlock.h"
#include "vsfs.h"
#include "tty.h"
#include "elf.h"
#include "stdarg.h"
#include "stdlib.h"
#include "syscall.h"
#include "serial.h"
#include "trap.h"
#include "panic.h"

extern pid_t next_pid;
extern slock_t ptable_lock; /* Process table lock */
extern struct proc ptable[]; /* The process table */
extern struct proc* rproc; /* The currently running process */

int (*syscall_table[])(void) = {
	NULL, /* There is no system call 0 */
	sys_fork,
	sys_wait,
	sys_exec,
	sys_exit,
	sys_open,
	sys_close,
	sys_read,
	sys_write,
	sys_lseek,
	sys_mmap,
	sys_chdir,
	sys_getcwd,
	sys_create,
	sys_mkdir,
	sys_rmdir,
	sys_unlink,
	sys_mv,
	sys_fstat,
	sys_wait_s,
	sys_wait_t,
	sys_signal_cv,
	sys_readdir,
	sys_pipe,
	sys_dup,
	sys_dup2,
	sys_proc_dump,
	NULL,// sys_tty_mode,
	NULL,// sys_tty_screen,
	NULL,// sys_tty_cursor,
	sys_brk,
	sys_sbrk,
	sys_chmod,
	sys_chown,
	sys_mprotect,
	sys__exit,
	sys_execve,
	sys_getpid,
	sys_isatty, /* Specific to newlib */
	sys_kill,
	sys_link,
	sys_stat,
	sys_times,
	sys_gettimeofday,
	sys_waitpid,
	sys_create, /* sys_creat == sys_create */
	sys_getuid,
	sys_setuid,
	sys_getgid,
	sys_setgid,
	sys_gettid,
	sys_getppid,
	sys_munmap,
	sys_getdents,
	sys_getegid,
	sys_geteuid,
	sys_ioctl,
	sys_access,
	sys_ttyname,
	sys_fpathconf,
	sys_pathconf,
	sys_sleep,
	sys_umask,
	sys_lstat,
	sys_fchown,
	sys_fchmod,
	sys_gethostname,
	sys_execl
};

/* int isatty(int fd); */
int sys_isatty(void)
{
	int fd;
	if(syscall_get_int(&fd, 0)) return 1;

	if(fd < 0 || fd > MAX_FILES) return 1;

	int x;
	for(x = 0;x < MAX_TTYS;x++)
	{
		tty_t t = tty_find(x);
		if(t->driver == rproc->file_descriptors[fd].device)
			return 1;
	}
	
	return 0;
}

int syscall_handler(uint* esp)
{
	/* Get the number of the system call */
	int syscall_number = -1;
	int return_value = -1;
	rproc->sys_esp = esp;
	if(syscall_get_int(&syscall_number, 0)) return 1;
	esp += 2;
	rproc->sys_esp = esp;

	//cprintf("%s: Syscall: 0x%x\n", rproc->name, syscall_number);

	if(syscall_number > SYS_MAX || syscall_number < SYS_MIN)
	{
		cprintf("kernel syscall number invalid: 0x%x\n", 
				syscall_number);
		return -1;
	}

	if(!syscall_table[syscall_number])
	{
		cprintf("Warning: invalid system call: 0x%x\n", 
			syscall_number);
		return -1;
	}
	return_value = syscall_table[syscall_number]();

	//cprintf("syscall return value: 0x%x\n", return_value);

	return return_value; /* Syscall successfully handled. */
}

/* int wait_s(struct cond* c, struct slock* lock) */
int sys_wait_s(void)
{	
	struct cond* c;
	struct slock* lock;

	if(syscall_get_buffer_ptr((void**)&c, sizeof(struct cond), 0))
		return -1;
	if(syscall_get_buffer_ptr((void**)&lock, sizeof(struct slock), 1))
		return -1;

	slock_acquire(&ptable_lock);
	if(c->next_signal > c->current_signal){
		c->next_signal = 0;
		c->current_signal = 0;
	}

	rproc->block_type = PROC_BLOCKED_COND;
	rproc->b_condition = c;
	rproc->state = PROC_BLOCKED;
	rproc->b_condition_signal = c->current_signal++;
	slock_release(lock);
	slock_release(&ptable_lock);
	return 0;
}

/* int wait_t(struct cond* c, struct tlock* lock) */
int sys_wait_t(void)
{
	struct cond* c;
	struct tlock* lock;

	if(syscall_get_buffer_ptr((void**)&c, sizeof(struct cond), 0))
		return -1;
	if(syscall_get_buffer_ptr((void**)&lock, sizeof(struct tlock), 1))
		return -1;

	slock_acquire(&ptable_lock);
	if(c->next_signal>c->current_signal){
		c->next_signal = 0;
		c->current_signal = 0;
	}
	rproc->block_type = PROC_BLOCKED_COND;
	rproc->b_condition = c;
	rproc->state = PROC_BLOCKED;
	rproc->b_condition_signal = c->current_signal++;
	tlock_release(lock);
	slock_release(&ptable_lock);
	return 0;
}



/* int signal_cv(struct cond* c) */
int sys_signal_cv(void)
{
	struct cond* c;
	if(syscall_get_buffer_ptr((void**)&c, sizeof(struct cond), 0))
		return -1;

	slock_acquire(&ptable_lock);
	int i;
	for(i = 0; i< PTABLE_SIZE; i++){
		if(ptable[i].state != PROC_BLOCKED) continue;
		if(ptable[i].block_type != PROC_BLOCKED_COND) continue;
		if(ptable[i].b_condition != c) continue;
		if(c->next_signal != ptable[i].b_condition_signal) continue;

		ptable[i].state = PROC_RUNNABLE;
		ptable[i].block_type = PROC_BLOCKED_NONE;
		ptable[i].b_condition_signal = 0;
		c->next_signal++;
		break;
	}

	slock_release(&ptable_lock);
	return 0;
}

extern uint k_start_pages;
extern uint k_pages;
int sys_proc_dump(void)
{
	/*
	tty_print_string(rproc->t, "Running processes:\n");
	int x;
	for(x = 0;x < PTABLE_SIZE;x++)
	{
		struct proc* p = ptable + x;
		if(p->state == PROC_UNUSED) continue;
		tty_print_string(rproc->t, "++++++++++++++++++++\n");
		tty_print_string(rproc->t, "Name: %s\n", p->name);
		tty_print_string(rproc->t, "DIR: %s\n", p->cwd);
		tty_print_string(rproc->t, "PID: %d\n", p->pid);
		tty_print_string(rproc->t, "UID: %d\n", p->uid);
		tty_print_string(rproc->t, "GID: %d\n", p->gid);

		tty_print_string(rproc->t, "state: ");
		switch(p->state)
		{
			case PROC_EMBRYO:	
				tty_print_string(rproc->t, "EMBRYO");
				break;
			case PROC_READY:	
				tty_print_string(rproc->t, "READY TO RUN");
				break;
			case PROC_RUNNABLE:	
				tty_print_string(rproc->t, "RUNNABLE");
				break;
			case PROC_RUNNING:	
				tty_print_string(rproc->t, "RUNNING - "
						"CURRENT PROCESS");
				break;
			case PROC_BLOCKED:	
				tty_print_string(rproc->t, "BLOCKED\n");
				tty_print_string(rproc->t, "Reason: ");
				if(p->block_type == PROC_BLOCKED_WAIT)
				{
					tty_print_string(rproc->t, "CHILD WAIT");
				} else if(p->block_type == PROC_BLOCKED_COND)
				{
					tty_print_string(rproc->t, "CONDITION");
				} else {

					tty_print_string(rproc->t, "INDEFINITLY\n");
				}
				break;
			case PROC_ZOMBIE:	
				tty_print_string(rproc->t, "ZOMBIE");
				break;
			case PROC_KILLED:	
				tty_print_string(rproc->t, "KILLED");
				break;
			default: 
				tty_print_string(rproc->t, "CURRUPT!");
		}
		tty_print_string(rproc->t, "\n");

		tty_print_string(rproc->t, "Open Files:\n");
		int file;
		for(file = 0;file < MAX_FILES;file++)
		{
			struct file_descriptor* fd = 
				p->file_descriptors + file;
			if(fd->type == 0x0) continue;
			tty_print_string(rproc->t, "%d: ", file);
			switch(fd->type)
			{
				case FD_TYPE_FILE:
					tty_print_string(rproc->t, "FILE");
					break;
				case FD_TYPE_DEVICE:
					tty_print_string(rproc->t, "DEVICE");
					break;
				case FD_TYPE_PIPE:
					tty_print_string(rproc->t, "PIPE");
					break;
				default:
					tty_print_string(rproc->t, 							"CURRUPT");
			}
			tty_print_string(rproc->t, "\n");

		}
		tty_print_string(rproc->t, "Virtual Memory: ");
		uint stack = p->stack_start - p->stack_end;
		uint heap = p->heap_end - p->heap_start;
		uint code = p->heap_start - 0x1000;
		uint total = (stack + heap + code) / 1024;
		tty_print_string(rproc->t, "%dK\n", total);	
	}

	tty_print_string(rproc->t, "\n");

	fs_fsstat();

	int divisor = 1024 * 1024;
	uint used_pages = k_start_pages - k_pages;
	tty_print_string(rproc->t, "Used memory:  %dM\n", 
		(used_pages * PGSIZE) / divisor);
	tty_print_string(rproc->t, "Free memory:  %dM\n", 
                (k_pages * PGSIZE) / divisor);
	tty_print_string(rproc->t, "Total memory: %dM\n", 
                (k_start_pages * PGSIZE) / divisor);
	tty_print_string(rproc->t, "Used percent:  %d%%\n",
                (used_pages * 100) / k_start_pages);
        tty_print_string(rproc->t, "Free percent:  %d%%\n",
                (k_pages * 100) / k_start_pages); */
	return 0;
}

/* int mprotect(void* addr, size_t len, int prot)*/
int sys_mprotect(void)
{
	void* addr;
	size_t len;
	int prot;

	if(syscall_get_int((int*)&addr, 0)) return -1;
	if(syscall_get_int((int*)&len, 1)) return -1;
	if(syscall_get_int(&prot, 2)) return -1;

	size_t start = (int)addr;
	/* Start address must be page aligned.*/
	if(PGROUNDDOWN(start) != start) return -1;
	/* Start must be greater than 0x0 */
	if(start == 0x0) return -1;
	/* Start must be in the user address space */
	if(start >= UVM_KVM_S) return -1;

	size_t end = (int)(start + len);

	uchar flags = 0;
	if(prot & PROT_READ) flags |= 0;
	if(prot & PROT_WRITE) flags |= PGTBL_RW;
	if(prot & PROT_READ) flags |= 0;


	size_t x;
	for(x = start;x < end;x += PGSIZE)
		pgflags(x, rproc->pgdir, 1, flags);

	return 0;
}


