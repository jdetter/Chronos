#define __X86_ASM_ONLY__
#include "x86.h"

/* unsigned int x86_get_cr0(void) */
.globl x86_get_cr0
x86_get_cr0:
        movl    %cr0, %eax
        ret

/* unsigned int x86_get_cr2(void) */
.globl x86_get_cr2
x86_get_cr2:
        movl    %cr2, %eax
        ret

/* unsigned int x86_get_cr3(void) */
.globl x86_get_cr3
x86_get_cr3:
        movl    %cr3, %eax
        ret

/* unsigned int x86_get_cr4(void) */
.globl x86_get_cr4
x86_get_cr4:
        movl    %cr4, %eax
        ret

/* int check_interrupt(void) */
.globl x86_check_interrupt
x86_check_interrupt:
        pushf
        xorl    %eax, %eax
        popl    %eax
        andl    $(EFLAGS_IF), %eax
        rorl    $0x09, %eax
        ret
