#include "asm.h"

#define _CHRONOS_ASMONLY_
#include "chronos.h"

# Load the kernel and jump into it.

# When the i386 starts, it will be in Real mode (16 bit). We need to
# disable interrupts and jump into protected mode.

#define KEYBOARD_DATA 0x60
#define KEYBOARD_COMM 0x64

#define KEYBOARD_DATA_FILL 	0x02
#define KEYBOARD_CONTROL_READ 	0xD0
#define KEYBOARD_CONTROL_WRITE 	0xD1
#define A20_ENABLE_FLAG 	0x02

#define PROTECT_EN 0x01

# segments in the gdt, see below
#define STAGE2_ADDR		0x7E00

#define KEYBOARD_LIGHT_COMM	0xED
#define KEYBOARD_SCROLL_LOCK 	0x0
#define KEYBOARD_SCROLL_NUM  	0x1
#define KEYBOARD_SCROLL_CAPS 	0x2

#define STAGE2_FINAL_SECT 	63

.globl ata_readsect
.globl ata_init

.code16			# Start generating 16 bit code. (Real mode)
.globl start
start:

# Disable interrupts
	cli

# Disable Segmentation for now
	xorw	%ax, %ax
	movw	%ax, %ss # Stack Segment
	movw	%ax, %es # Extra Segment
	movw	%ax, %ds # Data Segment

	xorl	%esp, %esp
	movw	$0x7c00, %sp

	#call	KEYBOARD_NONE_16

A20_STEP1:
  	inb     $KEYBOARD_COMM, %al
  	testb   $KEYBOARD_DATA_FILL, %al
  	jnz     A20_STEP1

  	movb    $KEYBOARD_CONTROL_WRITE, %al
  	outb    %al, $KEYBOARD_COMM

A20_STEP2:
  	inb     $KEYBOARD_COMM, %al
  	testb   $KEYBOARD_DATA_FILL, %al
  	jnz     A20_STEP2

  	movb    $0xdf, %al
  	outb    %al, $KEYBOARD_DATA

	jmp	set_A20_DONE

set_A20_DONE:
# We should have access to the 2^16 address space now.

# Lets let the global descriptor table register and jump into protected mode
	lgdt 	gdt_descriptor	
	
# Enable protected (32 bit) mode
	movl	%cr0, %eax
	orl	$PROTECT_EN, %eax
	movl	%eax, %cr0
	call	KEYBOARD_ALL_16

# Long jump into 32 bit mode =D
	ljmp	$(SEG_KERNEL_CODE << 3), $protected_mode	

KEYBOARD_WAIT_16:
	inb     $KEYBOARD_COMM, %al
        testb   $KEYBOARD_DATA_FILL, %al
        jnz     KEYBOARD_WAIT_16
        ret

KEYBOARD_ALL_16:
        call    KEYBOARD_WAIT_16
        movb    $KEYBOARD_LIGHT_COMM, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT_16
        movb    $0x07, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT_16
        ret

KEYBOARD_NONE_16:
        call    KEYBOARD_WAIT_16
        movb    $KEYBOARD_LIGHT_COMM, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT_16
        movb    $0x00, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT_16
        ret

.code32			# Generate 32 bit code (Protected mode)

KEYBOARD_WAIT:
        inb     $KEYBOARD_COMM, %al
        testb   $KEYBOARD_DATA_FILL, %al
        jnz     KEYBOARD_WAIT
        ret

KEYBOARD_SCROLL:
        call    KEYBOARD_WAIT
        movb    $KEYBOARD_LIGHT_COMM, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT
        movb    $0x01, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT
        ret

KEYBOARD_NUM:
        call    KEYBOARD_WAIT
        movb    $KEYBOARD_LIGHT_COMM, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT
        movb    $0x02, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT
        ret

KEYBOARD_CAPS:
        call    KEYBOARD_WAIT
        movb    $KEYBOARD_LIGHT_COMM, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT
        movb    $0x04, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT
        ret

KEYBOARD_RESET:
        call    KEYBOARD_WAIT
        movb    $KEYBOARD_LIGHT_COMM, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT
        movb    $0x00, %al
        outb    %al, $KEYBOARD_DATA
        call    KEYBOARD_WAIT
        ret

protected_mode:
	# Setup the data selectors with the correct index into the gdt.
	movw    $(SEG_KERNEL_DATA << 3), %ax    # Our data segment selector
  	movw    %ax, %ds                # -> DS: Data Segment
  	movw    %ax, %es                # -> ES: Extra Segment
  	movw    %ax, %ss                # -> SS: Stack Segment
  	movw    $0, %ax                 # Zero segments not ready for use
  	movw    %ax, %fs                # -> FS
  	movw    %ax, %gs                # -> GS

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

	cmpl 	$STAGE2_FINAL_SECT, %eax
	jne 	READ_SECT
STAGE2_JMP:
	ljmp    $(SEG_KERNEL_CODE << 3), $STAGE2_ADDR

	jmp PANIC_32

PANIC_32:
	nop
	call KEYBOARD_NUM
	jmp PANIC_32

.p2align 2 # All values here are 4 byte-boundary aligned.
# Our Global Descriptor table for boot-stage1
gdt_stage1:
  SEG_NULLASM                            	# null seg
  SEG_ASM(SEG_EXE, SEG_READ, SEG_KERN, 0x0, 0xffffffff)   # code seg
  SEG_ASM(SEG_DATA, SEG_WRITE, SEG_KERN, 0x0, 0xffffffff) # data seg

gdt_descriptor:
	.word (gdt_descriptor - gdt_stage1 - 1) 	# 2 bytes
	.long gdt_stage1			# 4 bytes
