#define __ASM_ONLY__
#include "x86.h"
#include "trap.h"

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

# void x86_drop_priv(context_t* k_context, uintptr_t new_esp, void* new_eip);
.globl x86_drop_priv
x86_drop_priv:
# Kernel context must be saved here!
        movl    0x04(%esp), %eax # context pointer

# now put stack pointer argument into ecx
        movl    0x08(%esp), %ecx

# put the new eip value into edx
        movl    0x0C(%esp), %edx

# complete context creation, similiar to mktf

# Push registers
        pushal
/* save eip in edi */
        movl    %edx, %edi

# Push pdbr
        movl    %cr3, %edx
        pushl   %edx
# Set context pointer
        movl    %esp, (%eax)

# now we're ready to start the init process.
        movw    $(SEG_USER_DATA << 3), %ax
        addw    $0x03, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %gs

# put the value of the new stack in %ebx
        movl    %ecx, %ebx

# value for the new CS
        movl    $SYS_EXIT_BASE, %eax
        addl    $0x03, %eax
        movl    $0x00, %edx
        movl    $SYSENTER_CS_MSR, %ecx
        wrmsr

/* Restore eip value */
        movl    %edi, %edx

# ebx is holding the new esp, move it to %ecx.
        movl    %ebx, %ecx

        sysexit

# void context_switch(uintptr_t current, uintptr_t next)
.globl x86_context_switch
x86_context_switch:
# First, get current context pointer.
        movl    0x4(%esp), %eax
# get the new context value (not a pointer!)
        movl    0x8(%esp), %ecx

# Save all general registers
        pushal

# Save pdbr
        movl    %cr3, %ebx
        pushl   %ebx
# update context pointer.
        movl    %esp, (%eax)

# Old context is safe, lets enter the new context.
        movl    %ecx, %esp

        popl    %eax
        movl    %eax, %cr3
        popal

# return 0
        movl    $0x00, %eax
        ret

.globl x86_reboot
x86_reboot:
	ret

.globl x86_shutdown
x86_shutdown:
	hlt
