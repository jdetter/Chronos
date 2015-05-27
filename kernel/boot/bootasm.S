#include "asm.h"

# Load the kernel and jump into it.

# When the i386 starts, it will be in Real mode (16 bit). We need to
# disable interrupts and jump into protected mode.

#define PORT_A20 	$0x92
#define PORT_A20_EN	$0x02

#define PROTECT_EN	$0x1

# segments in the gdt, see below
#define KERNEL_CODE	0x1
#define KERNEL_DATA	0x2
#define STAGE2_ADDR	0x7E00

#define STAGE2_FINAL_SECT 63

.globl ata_readsect
.globl ata_init

.code16			# Start generating 16 bit code. (Real mode)
.globl start
start:

# Disable interrupts
	cli

# Disable Segmentation for now
	xor	%ax, %ax
	mov	%ax, %ss # Stack Segment
	mov	%ax, %es # Extra Segment
	mov	%ax, %ds # Data Segment

# First test the A20 gate
	inb 	PORT_A20, %al
        test	 $2, %al
	jnz 	set_A20_DONE

# Ask the bios to activate the A20 line on our behalf.
A20_BIOS:
	mov 	$0x2401, %ax
	int 	$0x15

# Retest A20
        inb     PORT_A20, %al
        test     $2, %al
        jnz     set_A20_DONE

# That didn't work, try the keyboard method.
KEYBOARD1:
  	inb    	$0x64,%al               # Wait for not busy
	testb  	$0x2,%al
	jnz    	KEYBOARD1

  	movb   	$0xd1,%al               # 0xd1 -> port 0x64
  	outb   	%al,$0x64

KEYBOARD2:
  	inb    	$0x64,%al               # Wait for not busy
  	testb  	$0x2,%al
  	jnz    	KEYBOARD2

  	movb    $0xdf,%al               # 0xdf -> port 0x60
  	outb    %al,$0x60

# Retest A20
        inb     PORT_A20, %al
        test     $2, %al
        jnz     set_A20_DONE

# Nothing worked, panic
	jmp 	PANIC_16

set_A20_DONE:
# We should have access to the 2^16 address space now.

# Lets let the global descriptor table register and jump into protected mode
	lgdt 	gdt_descriptor
	
# Enable protected (32 bit) mode
	mov	%cr0, %eax
	or	PROTECT_EN, %eax
	mov	%eax, %cr0

# Long jump into 32 bit mode =D
	ljmp	$(KERNEL_CODE << 3), $protected_mode	

PANIC_16:
	jmp PANIC_16

.code32			# Generate 32 bit code (Protected mode)
protected_mode:
	# We need a couple of function calls here, so lets make a stack.
	movl 	$start, %esp
	movl 	%esp, %ebp

	subl 	$4, %esp # local for our current sector
	subl 	$4, %esp # local for our current location in memory
	
	movl 	$STAGE2_ADDR, %eax # initial location in memory
	movl 	%eax, (%esp)
	movl	$1, 0x4(%esp) # initial sector we need to read

READ_SECT:
	movl 	-4(%ebp), %eax # current sector we are reading 
	movl 	-8(%ebp), %ecx # current location in memory
	
	subl 	$8, %esp
	movl 	%eax, (%esp)
	movl 	%ecx, 4(%esp)
	call 	ata_readsect

	# get rid of stack parameters
	addl 	$8, %esp
	
	# increment address and sector.
	movl 	-4(%ebp), %eax # current sector we are reading 
        movl 	-8(%ebp), %ecx # current location in memory

	addl 	$1, %eax
	addl 	$512, %ecx

	# Save values
	movl 	%eax, -4(%ebp) # current sector we are reading 
        movl 	%ecx, -8(%ebp) # current location in memory

	test 	$STAGE2_FINAL_SECT, %eax
	jne 	READ_SECT

	ljmp    $(KERNEL_CODE << 3), $0x100000

	jmp PANIC_32

PANIC_32:
	nop
	jmp PANIC_32


.p2align 2 # All values here are 4 byte-boundary aligned.
# Our Global Descriptor table for boot-stage1
gdt_stage1:
	ASM_GDT_NULL_ENTRY # NULL Segment 
	ASM_GDT_ENTRY(0x0, 0xFFFFFFFF, 1, 1, 0) # Kernel stack segment
	ASM_GDT_ENTRY(0x0, 0xFFFFFFFF, 0, 1, 0) # Kernel data segment

gdt_descriptor:
	.word (gdt_stage1 - gdt_stage1 - 1) 	# 2 bytes
	.long gdt_stage1			# 4 bytes
