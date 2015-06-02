/* __enable_paging__(uint* pgdir)*/

#define CR0_PGENABLE (1<<31)
.globl __enable_paging__
__enable_paging__:
	push	%ebp
	movl	%esp, %ebp

	movl	12(%ebp), %eax
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
