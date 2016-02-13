.text
.globl _start
_start:
	pushl 	%ebp
	movl 	%esp, %ebp

	pushl	$17
	pushl	$str
	pushl 	$0x01
	call 	sys_write

	pushl	$0x0
	call 	sys_exit

loop:
	jmp loop

# System calls 
.globl sys_write
sys_write:
	pushl   $0x08
	int     $0x80
	addl	$0x04, %esp
	ret

.globl sys_exit
sys_exit:
        pushl   $0x04
        int     $0x80
        addl    $0x04, %esp
        ret

.data
str:
.string "program running!\n"
