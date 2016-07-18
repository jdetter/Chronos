#define __ASM_ONLY__
#include "vm.h"
#include "x86.h"

/* void vm_enable_paging(pgdir_t* pgdir)*/
.globl vm_enable_paging
vm_enable_paging:
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

/* void vm_diable_paging(void) */
.globl vm_disable_paging
vm_disable_paging:
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

/* pgdir* vm_curr_pgdir(void) */
.globl vm_curr_pgdir
vm_curr_pgdir:
        movl    %cr3, %eax
        ret

/* void vm_enforce_kernel_readonly(void) */
.globl vm_enforce_kernel_readonly
vm_enforce_kernel_readonly:
        pushl   %ebp
        movl    %esp, %ebp

        movl    %cr0, %eax
        orl     $CR0_WP, %eax
        movl    %eax, %cr0

        movl    %ebp, %esp
        popl    %ebp

        ret

/* int vm_check_paging(void) */
.globl vm_check_paging
vm_check_paging:
        movl    %cr0, %eax
        andl    $CR0_PGENABLE, %eax
        roll    $1, %eax
        ret

# void vm_set_stack(uintptr_t stack, void* callback)
.globl vm_set_stack
vm_set_stack:
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

        /* Return to calling function. ECX -> EIP */
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

# void __kvm_swap__(pgdir_t* kvm)
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
	/* flush TLB */
	movl	%cr3, %eax
	movl	%eax, %cr3

        popl    %esi
        popl    %edi
        popl    %ebx
        popl    %edx
        popl    %ebp
        ret
