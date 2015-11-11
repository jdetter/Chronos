.text
.globl _start
_start:
	pushl 	%ebp
	movl 	%esp, %ebp

	pushl	$17
	pushl	$str
	pushl 	$0x01
	call 	sys_write

loop:
	jmp loop

.globl sys_write
sys_write:
	pushl   $0x08
	int     $0x80
	addl	$0x04, %esp
	ret

.globl linux_write
linux_write:
	pushl	%ebx
	pushl	%edx

	movl	$0x04, %eax
	movl	$0x01, %ebx
	movl	$str, %ecx
	movl	$17, %edx
	int	$0x80

	popl	%edx
	popl	%ebx
	ret

.eject

.data
str:
.string "program running!\n"
