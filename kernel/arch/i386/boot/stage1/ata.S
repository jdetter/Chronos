
#define PRIMARY_ATA_DATA 0x1F0
#define PRIMARY_ATA_FEATURES_ERROR 0x1F1
#define PRIMARY_ATA_SECTOR_COUNT 0x1F2
#define PRIMARY_ATA_SECTOR_NUMBER 0x1F3
#define PRIMARY_ATA_CYLINDER_LOW 0x1F4
#define PRIMARY_ATA_CYLINDER_HIGH 0x1F5
#define PRIMARY_ATA_DRIVE 0x1F6
#define PRIMARY_ATA_COMMAND 0x1F7

#define ATA_ERR 0x0
#define ATA_DRQ (0x1 << 3)
#define ATA_SRV (0x1 << 4)
#define ATA_DF (0x1 << 5)
#define ATA_RDY (0x1 << 6)
#define ATA_BSY (0x1 << 7)

#define ATA_RDY_OR_BSY 0xC0

#define SECT_SZ 512

# void ata_wait(void)
.globl ata_wait
ata_wait:
	pushl 	%edx
ata_wait_loop:
	movl	$PRIMARY_ATA_COMMAND, %edx
	inb		%dx
	movl 	$ATA_RDY_OR_BSY, %ecx
	andl 	%eax, %ecx
	movl 	$ATA_RDY, %eax
	cmpl 	%eax, %ecx
	jne		ata_wait_loop

	popl	%edx
	ret

# int ata_readsect(sect_t sector, void* dst)
.globl ata_readsect
ata_readsect:
	pushl 	%ebp
	movl 	%esp, %ebp
	pushl	%edx
	pushl	%edi

	call 	ata_wait

	movl	$0x1, %eax
	movl	$PRIMARY_ATA_SECTOR_COUNT, %edx
	out		%al, %dx

	# Put the register count into eax
	movl	0x08(%ebp), %eax
	movl	$PRIMARY_ATA_SECTOR_NUMBER, %edx
	out		%al, %dx

	# Shift right by 8
	movl	0x08(%ebp), %eax
	shrl	$0x08, %eax
	movl	$PRIMARY_ATA_CYLINDER_LOW, %edx
	out		%al, %dx

	# Shift right by 8
	movl	0x08(%ebp), %eax
	shrl	$0x10, %eax
	movl	$PRIMARY_ATA_CYLINDER_HIGH, %edx
	out		%al, %dx

	# Shift right by 8 and and with 0xE0 (1110 0000)
	# movl	0x08(%ebp), %eax
	# shrl	$0x18, %eax
	# andl	$0xE0, %eax
	# movl	$PRIMARY_ATA_CYLINDER_HIGH, %edx
	# out		%al, %dx

	# Send the command
	movl	$0x20, %eax
	movl	$PRIMARY_ATA_COMMAND, %edx
	out		%al, %dx

	# wait for the disk to be ready
	call 	ata_wait

	movl	$128, %ecx
	movl	0x0C(%ebp), %edi
	movl	$PRIMARY_ATA_DATA, %edx

	cld
	rep insl

	popl	%edi
	popl 	%edx

	movl	%ebp, %esp
	popl 	%ebp
	ret
