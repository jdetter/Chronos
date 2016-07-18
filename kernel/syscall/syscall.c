#include <stdlib.h>
#include <string.h>

#include "kstdlib.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"
#include "devman.h"
#include "tty.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "chronos.h"
#include "cpu.h"
#include "tty.h"
#include "elf.h"
#include "stdarg.h"
#include "syscall.h"
#include "panic.h"
#include "reboot.h"

// #define DEBUG

#ifdef RELEASE
# undef DEBUG
#endif

extern int k_start_pages;
extern int k_pages;

int (*syscall_table[])(void) = {
	NULL, /* There is no system call 0 */
	sys_fork,
	sys_wait,
	NULL, // sys_exec,
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
	sys_execl,
	sys_utime,
	sys_utimes,
	sys_fcntl,
	sys_sysconf,
	sys_ftruncate,
	sys_execvp,
	sys_getpgid,
	sys_getresuid,
	sys_getresgid,
	sys_setresuid,
	sys_setpgid,
	sys_setresgid,
	sys_vfork,
	sys_select,
	sys_alarm,
	NULL,
	sys_sigaction,
	sys_sigprocmask,
	sys_sigpending,
	sys_signal,
	sys_sigsuspend,
	sys_setegid,
	sys_sync,
	sys_setreuid,
	sys_setregid,
	sys_reboot
};

char* syscall_table_names[] = {
	NULL, /* There is no system call 0 */
	"fork",
	"wait",
	"exec",
	"exit",
	"open",
	"close",
	"read",
	"write",
	"lseek",
	"mmap",
	"chdir",
	"getcwd",
	"create",
	"mkdir",
	"rmdir",
	"unlink",
	"mv",
	"fstat",
	"wait_s",
	"wait_t",
	"signal_cv",
	"readdir",
	"pipe",
	"dup",
	"dup2",
	"proc_dump",
	NULL,// sys_tty_mode,
	NULL,// sys_tty_screen,
	NULL,// sys_tty_cursor,
	"brk",
	"sbrk",
	"chmod",
	"chown",
	"mprotect",
	"_exit",
	"execve",
	"getpid",
	"isatty", /* Specific to newlib */
	"kill",
	"link",
	"stat",
	"times",
	"gettimeofday",
	"waitpid",
	"create", /* sys_creat == sys_create */
	"getuid",
	"setuid",
	"getgid",
	"setgid",
	"gettid",
	"getppid",
	"munmap",
	"getdents",
	"getegid",
	"geteuid",
	"ioctl",
	"access",
	"ttyname",
	"fpathconf",
	"pathconf",
	"sleep",
	"umask",
	"lstat",
	"fchown",
	"fchmod",
	"gethostname",
	"execl",
	"utime",
	"utimes",
	"fcntl",
	"sysconf",
	"ftruncate",
	"execvp",
	"getpgid",
	"getresuid",
	"getresgid",
	"setresuid",
	"setpgid",
	"setresgid",
	"vfork",
	"select",
	"alarm",
    "seteuid",
    "sigaction",
    "sigprocmask",
    "sigpending",
    "signal",
    "sigsuspend",
    "setegid",
    "sync",
	"setreuid",
	"setregid",
	"reboot"
};


int syscall_handler(int* esp)
{
	/* Get the number of the system call */
	int syscall_number = -1;
	int return_value = -1;
	rproc->sys_esp = esp;
	if(syscall_get_int(&syscall_number, 0)) return 1;
	esp += 2;
	rproc->sys_esp = esp;

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

#ifdef DEBUG
	if(syscall_table_names[syscall_number])
	{
		cprintf("%s:%d: syscall: %s\n", rproc->name, rproc->pid, 
				syscall_table_names[syscall_number]);
	} else cprintf("syscall: no information for this syscall.\n");

	fs_sync();
#endif

	return_value = syscall_table[syscall_number]();

#ifdef DEBUG
	cprintf("%s:%d: syscall: return value: %d\n", rproc->name,
		rproc->pid, return_value);
#endif

	return return_value; /* Syscall successfully handled. */
}

/* int isatty(int fd); */
int sys_isatty(void)
{
	int fd;
	if(syscall_get_int(&fd, 0)) return 1;
	if(!fd_ok(fd))
		return 0;

	int x;
	for(x = 0;x < MAX_TTYS;x++)
	{
		tty_t t = tty_find(x);
		if(t->driver == rproc->fdtab[fd]->device)
			return 1;
	}

	return 0;
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

/* int semctl(int semid, int semnum, int cmd, ...); */
int sys_semctl(void)
{
	int semid;
	int semnum;
	int cmd;
	
	if(syscall_get_int(&semid, 0))
		return -1;
	if(syscall_get_int(&semnum, 1))
		return -1;
	if(syscall_get_int(&cmd, 2))
		return -1;

	return -1;
}

/* int semget(key_t key, int nsems, int semflg); */
int sys_semget(void)
{
	int key;
	int nsems;
	int semflg;

	if(syscall_get_int(&key, 0))
                return -1;
        if(syscall_get_int(&nsems, 1))
                return -1;
        if(syscall_get_int(&semflg, 2))
                return -1;

	return -1;
}

/* int semop(int semid, struct sembuf *sops, size_t nsops); */
int sys_semop(void)
{
	return -1;
}

/* int semtimedop(int semid, struct sembuf *sops, size_t nsops,
                      const struct timespec *timeout); */
int sys_semtimedop(void)
{
	return -1;
}

int sys_proc_dump(void)
{
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

	unsigned char flags = 0;
	if(prot & PROT_READ) flags |= 0;
	if(prot & PROT_WRITE) flags |= VM_TBL_WRIT | VM_TBL_READ;
	if(prot & PROT_READ) flags |= 0;


	size_t x;
	for(x = start;x < end;x += PGSIZE)
		vm_setpgflags(x, rproc->pgdir, flags);

	return 0;
}

/* int sys_reboot(int mode) */
int sys_reboot(void)
{
	int mode;
	if(syscall_get_int((int*)&mode, 0))
		return -1;

	switch(mode)
	{
		case CHRONOS_RB_REBOOT:
			cprintf("System is going down for reboot...\n");
			pre_shutdown();
			cpu_reboot();
			break;
		case CHRONOS_RB_SHUTDOWN:
			cprintf("System is shutting down...\n");
			pre_shutdown();
			cpu_shutdown();
			break;
		default:
			break;
	}

	return -1;
}
