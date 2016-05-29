#define __ASM_ONLY__
#include "trap.h"

# Make a trap frame on the current stack.
.globl tp_mktf
tp_mktf:
        # nested interrupts are currently unsupported.
        cli
        # Status information and trap number have already been pushed.
        # Push segment registers
        pushl   %ds
        pushl   %es
        pushl   %fs
        pushl   %gs

        # Push registers
        pushal

        # Setup kernel segments again
        movw    $(SEG_KERNEL_DATA << 3), %ax
        movw    %ax, %ss
        movw    %ax, %ds
        movw    %ax, %es
        # f and g segments are unused right now.
        movw    %ax, %fs
        movw    %ax, %gs

	# move to the next 512 byte boundary
	andl	$~0x1FF, %esp # Round down to 512 byte boundary
	subl	$0x200, %esp  # Subtract 512

	# Save the floating point registers
	fxsave	(%esp)
	
	# push the saved esp (trap frame without floats)
	pushl	%esp
        call    trap_handler

.globl tp_fork_return
tp_fork_return:
        nop
        nop
        nop
        nop

.globl tp_trap_return
tp_trap_return:
	# Restore the floating point registers
	fxrstor (%esp)

	# Add (floats + padding) = (512 + 436) = 0x3b4
	addl	$0x3b4, %esp

        # pop registers
        popal

        # pop segment registers
        popl    %gs
        popl    %fs
        popl    %es
        popl    %ds

        # adjust esp for trap number and error (see struct trap_frame)
        addl $0x08, %esp

        # Restore the stack pointer, esp and EFLAGS, finish context switch.
        iret

.globl tp_fake_trap
tp_fake_trap:
        pushl   %ebp
        movl    %esp, %ebp

# Arguments
        pushl $0x02

# Sycall number
        pushl $0x01

        pushl $0
        pushl %esp
        int $0x80

        movl    %ebp, %esp
        popl    %ebp

        ret
