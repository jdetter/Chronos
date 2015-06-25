#define __VM_ASM_ONLY__
#define __X86_ASM_ONLY__
#include "vm.h"
#include "x86.h"

/* __enable_paging__(uint* pgdir)*/
#define CR0_PGENABLE (1<<31)
.globl __enable_paging__
__enable_paging__:
        pushl   %ebp
        movl    %esp, %ebp

        movl    8(%ebp), %eax
        movl    %eax, %cr3 # set the pgdir base register

        # Enable paging
        movl    %cr0, %eax
        orl     $CR0_PGENABLE, %eax
        movl    %eax, %cr0

        movl    %ebp, %esp
        pop     %ebp
        ret

.globl __disable_paging__
__disable_paging__:
        pushl   %eax
        pushl   %ecx
        movl    %cr0, %eax
        movl    $CR0_PGENABLE, %ecx
        notl    %ecx
        andl    %ecx, %eax
        movl    %eax, %cr0
        popl    %ecx
        popl    %eax
        ret

# check to see if paging is enabled. 
.globl __check_paging__
__check_paging__:
        movl    %cr0, %eax
        andl    $CR0_PGENABLE, %eax
        roll    $1, %eax
        ret

#define EFLAGS_INT_EN (0x01 << 9)
.globl __check_interrupt__
__check_interrupt__:
        pushf
        xorl    %eax, %eax
        popl    %eax
        andl    $(EFLAGS_INT_EN), %eax
        rorl	$0x09, %eax
	ret

.globl __get_cr2__
__get_cr2__:
        movl    %cr2, %eax
        ret

.globl __get_cr3__
__get_cr3__:
        movl    %cr3, %eax
        ret

# void __set_stack__(uint stack, uint return_addr)
.globl __set_stack__
__set_stack__:
	movl	%esp, %ebp
        /* get the new stack */
        movl    4(%ebp), %eax
        /* get the new address */
        movl    8(%ebp), %ecx

        /* Set the new stack */
        movl    %eax, %esp
        movl    %eax, %ebp

        /* Push return value */
        pushl   %ecx

        /* Return to calling function. */
        ret

# uchar __kvm_stack_check__(void)
.globl __kvm_stack_check__
__kvm_stack_check__:
	cmp	$(KVM_KSTACK_S), %esp
	jl	__kvm_stack_check__fail
	cmp	$(PGROUNDUP(KVM_KSTACK_E)), %esp
	jge	__kvm_stack_check__fail
	movl	$0x01, %eax
	ret
__kvm_stack_check__fail:
	movl	$0x0, %eax
	ret

#define PGDIR_DIR_MASK	0xFFC00000
#define PGDIR_TBL_MASK 	0x003ff000
#define PGDIR_MASK	0xFFFFF000
# void __kvm_swap__(pgdir* kvm)
.globl __kvm_swap__
__kvm_swap__:
# move kernel page directory into ecx
        movl    0x04(%esp), %ecx
        pushl   %ebp
        movl    %esp, %ebp
        pushl   %edx
        pushl   %ebx
        pushl   %edi
        pushl   %esi
# WARNING: UNUSABLE STACK

	/* move old page directory into edi */
        movl    %cr3, %edi
	/* move kvm pgdir into cr3 */
        movl    %ecx, %cr3
	/* ecx is now free */

	/* move start virtual address into esi */
        movl    $(UVM_KSTACK_S), %esi

__kvm_swap__1:

	/* see if we are done */
        cmp    $(UVM_KSTACK_E), %esi
        jge     __kvm_swap__done

	/* put directory index into eax */
	movl	%esi, %eax
	andl	$(PGDIR_DIR_MASK), %eax
	shrl	$22, %eax

	/* Put src table address into ecx */
	movl	0x0(%edi, %eax, 0x04), %ecx
	andl	$(PGDIR_MASK), %ecx

	/* Put table index into eax */
	movl	%esi, %eax
	andl	$(PGDIR_TBL_MASK), %eax
	shrl	$12, %eax

	/* Put src into edx */
	movl	0x0(%ecx, %eax, 0x04), %edx
	/* eax, ebx, ecx are free : edx holds src page */

	/* Put directory index into eax */
	movl    %esi, %eax
        andl    $(PGDIR_DIR_MASK), %eax
        shrl    $22, %eax

	/* Put kernel table address into ecx */
	movl	%cr3, %ebx
	movl	0x0(%ebx, %eax, 0x04), %ecx
	andl	$(PGDIR_MASK), %ecx

	/* Put kernel table index into eax */
	movl	%esi, %eax
	andl	$(PGDIR_TBL_MASK), %eax
	shrl	$12, %eax

	/* Finally, install src into the table */
	movl	%edx, 0x0(%ecx, %eax, 0x04)

	/* Add page size to esi */
        addl    $(PGSIZE), %esi

	/* Start over. */
	jmp	__kvm_swap__1

__kvm_swap__done:
	/* Inner peace. */

	/* flush TLB */
	movl	%cr3, %eax
	movl	%eax, %cr3

        popl    %esi
        popl    %edi
        popl    %ebx
        popl    %edx
        popl    %ebp
        ret

