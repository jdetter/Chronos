#define  _CHRONOS_ASMONLY_
#include "chronos.h"

/* Create the system call code for each system call */
#define SYSCALL(name) \
	.globl name; \
	name: \
		movl $SYS_##name, %eax; \
		int $TRAP_SC; \
		ret

SYSCALL(fork)
SYSCALL(wait)
SYSCALL(exec)
SYSCALL(exit)
SYSCALL(open)
SYSCALL(close)
SYSCALL(read)
SYSCALL(write)
