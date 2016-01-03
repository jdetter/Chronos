#define __ASM_ONLY__
#include "trap.h"

# Make a trap frame on the current stack.
.globl mktf
mktf:
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

        pushl   %esp # start of the trap frame
        call    trap_handler

.globl fork_return
fork_return:
        nop
        nop
        nop
        nop

.globl trap_return
trap_return:
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

.globl fake_trap
fake_trap:
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
