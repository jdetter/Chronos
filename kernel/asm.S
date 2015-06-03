#define _CHRONOS_ASMONLY_
#include "chronos.h"

.globl trap_handler

/* __enable_paging__(uint* pgdir)*/
#define CR0_PGENABLE (1<<31)
.globl __enable_paging__
__enable_paging__:
	pushl	%ebp
	movl	%esp, %ebp

	movl	8(%ebp), %eax
	movl	%eax, %cr3 # set the pgdir base register

	# Enable paging
	movl	%cr0, %eax
	orl	$CR0_PGENABLE, %eax
	movl	%eax, %cr0

	movl	%ebp, %esp 
	pop	%ebp
	ret

.globl __set_stack__
__set_stack__:
	push	%ebp
	movl	%esp, %ebp

	/* get the new stack */
	movl	8(%ebp), %eax
	/* get the return address */
	movl	4(%ebp), %ecx

	/* Set the new stack */
	movl	%eax, %esp
	movl	%eax, %ebp

	/* Push return value */
	pushl	%ecx

	/* Return to calling function. */
	ret

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

.globl fake_trap
fake_trap:
	pushl 	%ebp
	movl	%esp, %ebp

# Arguments
	pushl $0x02

# Sycall number
	pushl $0x01

	pushl $0
	pushl %esp
	int $0x80
	
	movl	%ebp, %esp
	popl	%ebp

	ret
