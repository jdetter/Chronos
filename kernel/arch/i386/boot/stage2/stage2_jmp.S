# Jump into the kernel.
#define __ASM_ONLY__
#include "vm.h"

.code32		# Start generating 16 bit code. (Real mode)
/** __kernel_jmp(uint address) */
.globl __kernel_jmp__
__kernel_jmp__:
	movl	0x4(%esp), %eax
	movl	$BOOT2_S, %esp
	movl	%esp, %ebp
# do the jump
	jmp	*%eax
