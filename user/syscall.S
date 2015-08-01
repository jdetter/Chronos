#define  __CHRONOS_ASM_ONLY__
#include "chronos.h"

/* Create the system call code for each system call */
#define SYSCALL(name) \
	.globl name; \
	name: \
		pushl $SYS_##name; \
		int $TRAP_SC; \
		addl $0x04, %esp; \
		ret

SYSCALL(fork)
SYSCALL(wait)
SYSCALL(exec)
SYSCALL(exit)
SYSCALL(open)
SYSCALL(close)
SYSCALL(read)
SYSCALL(write)
SYSCALL(lseek)
SYSCALL(mmap)
SYSCALL(chdir)
SYSCALL(getcwd)
SYSCALL(create)
SYSCALL(creat)
SYSCALL(mkdir)
SYSCALL(rmdir)
SYSCALL(unlink)
SYSCALL(mv)
SYSCALL(fstat)
SYSCALL(wait_s)
SYSCALL(wait_t)
SYSCALL(signal_cv)
SYSCALL(readdir)
SYSCALL(pipe)
SYSCALL(dup)
SYSCALL(dup2)
SYSCALL(proc_dump)
SYSCALL(tty_mode)
SYSCALL(tty_screen)
SYSCALL(tty_cursor)
SYSCALL(brk)
SYSCALL(sbrk)
SYSCALL(chmod)
SYSCALL(chown)
SYSCALL(mprotect)
SYSCALL(_exit)
SYSCALL(execve)
SYSCALL(getpid)
SYSCALL(isatty)
SYSCALL(kill)
SYSCALL(link)
SYSCALL(stat)
SYSCALL(times)
SYSCALL(gettimeofday)
SYSCALL(waitpid)
SYSCALL(munmap)
SYSCALL(getdents)
