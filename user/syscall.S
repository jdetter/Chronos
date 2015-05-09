#include "chronos.h"
#include "traps.h"

/* Create the system call code for each system call */
#define SYSCALL(name) \
	.globl name; \
	name: \
		movl $SC_##name, %eax; \
		int $TRAP_SC; \
		ret
