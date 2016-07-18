#define __ASM_ONLY__
#include "x86.h"

# void fpu_init(void)
.globl fpu_init
fpu_init:
        pushl   %ebp
        movl    %esp, %ebp

        movl    %cr0, %eax
        # Clear the emulation bit
        andl    $~CR0_EM, %eax
        # Set the coprocessor bit
        orl     $CR0_MP, %eax
        movl    %eax, %cr0

        movl    %cr4, %eax
        orl     $CR4_OSFXSR, %eax
        orl     $CR4_OSXMMEXCPT, %eax
        movl    %eax, %cr4

        movl    %ebp, %esp
        popl    %ebp
        ret

# void fpu_reset(void)
.globl fpu_reset
fpu_reset:
        pushl   %ebp
        movl    %esp, %ebp

        fninit

        movl    %ebp, %esp
        popl    %ebp
        ret
