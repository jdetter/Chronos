# Jump into the kernel.
#define KSTACK_TMP		0x7C00
#define KERNEL_ADDR		0x100000

.code32		# Start generating 16 bit code. (Real mode)
.globl __kernel_jmp__
__kernel_jmp__:
	movl	$KSTACK_TMP, %esp
	movl	%esp, %ebp
	movl	$KERNEL_ADDR, %eax
# do the jump
	jmp	*%eax
