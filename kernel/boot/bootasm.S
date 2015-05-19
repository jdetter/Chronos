# Load the kernel and jump into it.

# When the i386 starts, it will be in Real mode (16 bit). We need to
# disable interrupts and jump into protected mode.

#define PORT_A20 	$0x92
#define PORT_A20_EN	$0x02

.code16			# Start generating 16 bit code. (Real mode)
.globl start
start:

# Disable interrupts
	cli

# Loop Forever(Test)
loop_forever:
	nop
	jmp loop_forever

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


PANIC_16:
	jmp PANIC_16

.code32			# Generate 32 bit code (Protected mode)
protected:

