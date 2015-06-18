# Specify build targets. Exclude the file extension (e.g. .c or .s)
KERNEL_OBJECTS := \
	main \
	proc \
	tty \
	panic \
	vm \
	kcond \
	syscall \
	trap \
	fsman

# Specify driver files. Exclude all file extensions
KERNEL_DRIVERS := \
	ata \
	keyboard \
	pic \
	pit \
	serial \
	console \
        vsfs

# Add kernel/ before all of the kernel targets
KERNEL_OBJECTS := $(addprefix kernel/, $(KERNEL_OBJECTS))
KERNEL_OBJECTS := $(addsuffix .o, $(KERNEL_OBJECTS))

# put drivers into the form kernel/drivers/file.o
KERNEL_DRIVERS := $(addprefix kernel/drivers/, $(KERNEL_DRIVERS))
KERNEL_DRIVERS := $(addsuffix .o, $(KERNEL_DRIVERS))

# Specify files to clean
KERNEL_CLEAN := \
        $(KERNEL_OBJECTS) \
        $(KERNEL_DRIVERS) \
        kernel/chronos.img \
        kernel/chronos.o \
        kernel/boot/bootasm.o \
	kernel/boot/ata-read.o \
        kernel/boot/boot-stage1.img \
        kernel/boot/boot-stage1.o \
	kernel/boot/bootc.o \
	kernel/boot/bootc_jmp.o \
	kernel/boot/boot-stage2.img \
	kernel/boot/boot-stage2.o \
	kernel/boot/boot-stage2.data \
	kernel/boot/boot-stage2.rodata \
	kernel/boot/boot-stage2.text \
	kernel/boot/boot-stage2.bss \
	kernel/asm.o \
	kernel/idt.S \
	kernel/idt.o \
	ata-read.sym \
	boot-stage1.sym \
	boot-stage2.sym \
	chronos.sym

# Include files
KERNEL_CFLAGS += -I include
# Include driver headers
KERNEL_CFLAGS += -I kernel/drivers
# Include kernel headers
KERNEL_CFLAGS += -I kernel/
# Include library files
KERNEL_CFLAGS += -I lib

# Don't link the standard library
KERNEL_LDFLAGS := -nostdlib
# Set the entry point
KERNEL_LDFLAGS += --entry=main
# Set the memory location where the kernel will be placed
KERNEL_LDFLAGS += --section-start=.text=0x0100000
# KERNEL_LDFLAGS += --omagic

BOOT_STAGE1_LDFLAGS := --section-start=.text=0x7c00 --entry=start

BOOT_STAGE2_CFLAGS  := -I kernel/drivers -I include  
BOOT_STAGE2_LDFLAGS := --section-start=.text=0x7E00 --entry=main \
		--section-start=.data=0xF200 \
		--section-start=.rodata=0xF600 \
		--section-start=.bss=0xFA00

kernel-symbols: kernel/chronos.o kernel/boot/ata-read.o
	$(OBJCOPY) --only-keep-debug kernel/chronos.o chronos.sym
	$(OBJCOPY) --only-keep-debug kernel/boot/boot-stage1.o boot-stage1.sym
	$(OBJCOPY) --only-keep-debug kernel/boot/boot-stage2.o boot-stage2.sym
	$(OBJCOPY) --only-keep-debug kernel/boot/ata-read.o ata-read.sym


kernel/chronos.o: kernel/idt.o libs $(KERNEL_OBJECTS) $(KERNEL_DRIVERS) kernel/asm.o
	$(LD) $(LDFLAGS) $(KERNEL_LDFLAGS) -o kernel/chronos.o $(KERNEL_OBJECTS) $(KERNEL_DRIVERS) $(LIBS) kernel/asm.o kernel/idt.o

kernel/boot/boot-stage1.img: kernel/boot/ata-read.o
	$(AS) $(ASFLAGS) $(BUILD_ASFLAGS) -I include -c -o kernel/boot/bootasm.o kernel/boot/bootasm.S
	$(LD) $(LDFLAGS) $(BOOT_STAGE1_LDFLAGS) -o kernel/boot/boot-stage1.o kernel/boot/bootasm.o kernel/boot/ata-read.o
	$(OBJCOPY) -S -O binary -j .text kernel/boot/boot-stage1.o kernel/boot/boot-stage1.img

kernel/boot/boot-stage2.img: libs $(KERNEL_DRIVERS)
	$(CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -I include/ -I kernel/ -c -o kernel/boot/bootc.o kernel/boot/bootc.c
	$(AS) $(ASFLAGS) $(BUILD_ASFLAGS) -c -o kernel/boot/bootc_jmp.o kernel/boot/bootc_jmp.S
	$(LD) $(LDFLAGS) $(BOOT_STAGE2_LDFLAGS) -o kernel/boot/boot-stage2.o kernel/boot/bootc.o $(LIBS) $(KERNEL_DRIVERS) kernel/boot/bootc_jmp.o 
	$(OBJCOPY) -O binary -j .text kernel/boot/boot-stage2.o kernel/boot/boot-stage2.text	
	$(OBJCOPY) -O binary -j .data kernel/boot/boot-stage2.o kernel/boot/boot-stage2.data
	$(OBJCOPY) -O binary -j .rodata kernel/boot/boot-stage2.o kernel/boot/boot-stage2.rodata	
	$(OBJCOPY) -O binary -j .bss kernel/boot/boot-stage2.o kernel/boot/boot-stage2.bss
	dd if=kernel/boot/boot-stage2.text of=kernel/boot/boot-stage2.img bs=512 count=58 seek=0
	dd if=kernel/boot/boot-stage2.data of=kernel/boot/boot-stage2.img bs=512 count=2 seek=58
	dd if=kernel/boot/boot-stage2.rodata of=kernel/boot/boot-stage2.img bs=512 count=2 seek=60
	dd if=kernel/boot/boot-stage2.bss of=kernel/boot/boot-stage2.img bs=512 count=2 seek=62

kernel/boot/ata-read.o:
	$(CC) $(CFLAGS) $(BUILD_CFLAGS) -I include -Os -c -o kernel/boot/ata-read.o kernel/boot/ata-read.c

kernel/idt.S: tools
	tools/bin/mkvect > kernel/idt.S
kernel/idt.o: kernel/idt.S
	$(AS) $(ASFLAGS) $(BUILD_ASFLAGS) -I include -c -o kernel/idt.o kernel/idt.S
kernel/asm.o:
	$(AS) $(ASFLAGS) $(BUILD_ASFLAGS) -I include -c -o kernel/asm.o kernel/asm.S

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) $(KERNEL_CFLAGS) $(BUILD_CFLAGS) -c -o $@ $<

kernel/drivers/%.o: kernel/drivers/%.c
	$(CC) $(CFLAGS) $(KERNEL_CFLAGS) $(BUILD_CFLAGS)-c -o $@ $<
