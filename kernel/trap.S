
#define _CHRONOS_ASMONLY_
#include "chronos.h"

.globl trap_handler

# Make a trap frame on the current stack.
.globl mktf
mktf:
	# Status information and trap number have already been pushed.
	# Push segment registers
	pushl 	%ds
	pushl 	%es
	pushl 	%fs
	pushl 	%gs
	# Push registers
	pushal

	# Setup kernel segments again
	movw 	$(SEG_KERNEL_DATA), %ax
	movw 	%ax, %ds
	movw 	%ax, %es
	# f and g segments are unused right now.
	movw 	$0, %ax
	movw 	%ax, %fs
	movw 	%ax, %gs

	pushl	%esp # start of the trap frame
	call	trap_handler

.globl trap_return
trap_return:
	# pop registers
	popal

	# pop segment registers
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds

	# adjust esp for trap number and error (see struct trap_frame)
	addl $0x8, %esp

	# Restore the stack pointer, esp and EFLAGS, finish context switch.
	iret
