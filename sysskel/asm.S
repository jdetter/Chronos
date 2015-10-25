.globl _start
_start:
	pushl %ebp
	movl %esp, %ebp
	
loop:
	jmp loop
