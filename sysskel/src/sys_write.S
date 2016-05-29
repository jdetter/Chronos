.globl write
write:
	pushl   $0x08
	int     $0x80
	addl	$0x04, %esp
	ret
