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
SYSCALL(cwd)
SYSCALL(create)
SYSCALL(mkdir)
SYSCALL(rmdir)
SYSCALL(rm)
SYSCALL(mv)
SYSCALL(fstat)
SYSCALL(wait_s)
SYSCALL(wait_t)
SYSCALL(signal)
SYSCALL(readdir)
SYSCALL(pipe)
SYSCALL(dup)
SYSCALL(dup2)
