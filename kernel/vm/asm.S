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

.globl __check_interrupt__
__check_interrupt__:
	
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


