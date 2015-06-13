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
	movw 	$(SEG_KERNEL_DATA << 3), %ax
	movw 	%ax, %ss
	movw 	%ax, %ds
	movw 	%ax, %es
	# f and g segments are unused right now.
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
	addl $0x08, %esp

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

# save current context, restore old context.
# void __context_restore__(uint* current, uint old)
.globl __context_restore__
__context_restore__:

# First, get current context pointer.
	movl 0x4(%esp), %eax

# save stack and base pointers.
	pushl	%esp
	pushl	%ebp

# get the new context value (not a pointer!)
	movl 	0x10(%esp), %ebp

# finish context save.

# Save all general registers
	pushal

# Save pdbr
	movl	%cr3, %ecx
	pushl	%ecx
# update context pointer.
	movl	%esp, (%eax)

# Old context is safe, lets enter the new context.
	movl	%ebp, %esp

	popl	%eax
	movl	%eax, %cr3
	popal
	popl	%ebp
	popl	%esp

# return 0
	movl	$0x00, %eax
	ret
		

# drop privilieges and start init process 
# void __drop_priv__(uint* k_context, uint new_esp);
.globl __drop_priv__
__drop_priv__:
# Kernel context must be saved here!
	movl	0x04(%esp), %eax # context pointer

# First, push esp, ebp
	pushl	%esp
	pushl	%ebp

# now put stack pointer argument into ebp
	movl	0x10(%esp), %ebp

# complete context creation, similiar to mktf
	
# Push registers
        pushal

# Push pdbr
	movl	%cr3, %ecx
	pushl	%ecx
# Set context pointer
	movl	%esp, (%eax)

# now we're ready to start the init process.
	movw	$(SEG_KERNEL_DATA << 3), %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
# The base pointer is holding the new esp.
	movl	%ebp, %esp
	movl	%esp, %ebp
#	movw	%ax, %ss
	ljmp	$(SEG_KERNEL_CODE << 3), $0x1000
	
