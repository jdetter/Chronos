#define __CHRONOS_ASM_ONLY__
#include "chronos.h"
#define __VM_ASM_ONLY__
#include "vm.h"

.globl trap_handler

# Make a trap frame on the current stack.
.globl mktf
mktf:
	# nested interrupts are currently unsupported.
	cli
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
	movl 	0x4(%esp), %eax
# get the new context value (not a pointer!)
	movl 	0x8(%esp), %ecx

# Save all general registers
	pushal

# Save pdbr
	movl	%cr3, %ebx
	pushl	%ebx
# update context pointer.
	movl	%esp, (%eax)

# Old context is safe, lets enter the new context.
	movl	%ecx, %esp

	popl	%eax
	movl	%eax, %cr3
	popal

# return 0
	movl	$0x00, %eax
	ret
	
# definitions for relevant MSRs:
#define SYSENTER_CS_MSR 	0x174
#define SYSENTER_ESP_MSR 	0x175
#define SYSENTER_EIP_MSR 	0x176

# drop privilieges and start init process 
# void __drop_priv__(uint* k_context, uint new_esp, uint new_eip);
.globl __drop_priv__
__drop_priv__:
# Kernel context must be saved here!
	movl	0x04(%esp), %eax # context pointer

# now put stack pointer argument into ecx
	movl	0x08(%esp), %ecx

# put the new eip value into edx
	movl	0x0C(%esp), %edx

# complete context creation, similiar to mktf
	
# Push registers
        pushal
/* save eip in edi */
	movl	%edx, %edi	

# Push pdbr
	movl	%cr3, %edx
	pushl	%edx
# Set context pointer
	movl	%esp, (%eax)

# now we're ready to start the init process.
	movw	$(SEG_USER_DATA << 3), %ax
	addw	$0x03, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	
# put the value of the new stack in %ebx
	movl	%ecx, %ebx

# value for the new CS
	movl	$SYS_EXIT_BASE, %eax
	addl	$0x03, %eax
	movl	$0x00, %edx
	movl	$SYSENTER_CS_MSR, %ecx
	wrmsr

/* Restore eip value */
	movl	%edi, %edx

# ebx is holding the new esp, move it to %ecx.
	movl	%ebx, %ecx

	sysexit
