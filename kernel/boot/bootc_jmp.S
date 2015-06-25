# Jump into the kernel.
#define __VM_ASM_ONLY__
#include "vm.h"

.code32		# Start generating 16 bit code. (Real mode)
.globl __kernel_jmp__
__kernel_jmp__:
	movl	$KVM_BOOT2_S, %esp
	movl	%esp, %ebp
	movl	$KVM_KERN_S, %eax
# do the jump
	jmp	*%eax
