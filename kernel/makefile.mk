# Specify build targets. Exclude the file extension (e.g. .c or .s)
KERNEL_OBJECTS := \
	main

# Specify driver files. Exclude all file extensions
KERNEL_DRIVERS := \
	ata \
	keyboard \
	pic\
	pit \
	serial \
	console

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
	kernel/boot/boot-stage2.img \
	kernel/boot/boot-stage2.o \
	kernel/boot/boot-stage2.data \
	kernel/boot/boot-stage2.rodata \
	kernel/boot/boot-stage2.text \
	kernel/boot/boot-stage2.bss \
        chronos.img \
	chronos.vdi \
	ata-read.sym \
	boot-stage1.sym \
	boot-stage2.sym \
	chronos.sym

# Include files
KERNEL_CFLAGS += -I include
# Include driver headers
KERNEL_CFLAGS += -I kernel/drivers
# Include library files
KERNEL_CFLAGS += -I lib

# Don't link the standard library
KERNEL_LDFLAGS := -nostdlib
# Set the entry point
KERNEL_LDFLAGS += --entry=main
# Set the memory location where the kernel will be placed
KERNEL_LDFLAGS += --section-start=.text=0x01000000
# KERNEL_LDFLAGS += --omagic

BOOT_STAGE1_LDFLAGS := --section-start=.text=0x7c00 --entry=start

BOOT_STAGE2_CFLAGS  := -I kernel/drivers -I include  
BOOT_STAGE2_LDFLAGS := --section-start=.text=0x100000 --entry=main \
		--section-start=.data=0x107400 \
		--section-start=.rodata=0x107800 \
		--section-start=.bss=0x107C00

kernel-symbols: kernel/chronos.o kernel/boot/ata-read.o
	$(OBJCOPY) --only-keep-debug kernel/chronos.o chronos.sym
	$(OBJCOPY) --only-keep-debug kernel/boot/boot-stage1.o boot-stage1.sym
	$(OBJCOPY) --only-keep-debug kernel/boot/boot-stage2.o boot-stage2.sym
	$(OBJCOPY) --only-keep-debug kernel/boot/ata-read.o ata-read.sym


chronos.img: kernel/boot/boot-stage1.img kernel/boot/boot-stage2.img kernel/chronos.o
	dd if=/dev/zero of=chronos.img bs=512 count=2048
	tools/bin/boot-sign kernel/boot/boot-stage1.img
	dd if=kernel/boot/boot-stage1.img of=chronos.img count=1 bs=512 conv=notrunc seek=0
	dd if=kernel/boot/boot-stage2.img of=chronos.img count=62 bs=512 conv=notrunc seek=1

kernel/chronos.o: $(LIBS) $(KERNEL_OBJECTS) $(KERNEL_DRIVERS)
	$(LD) $(LDFLAGS) $(KERNEL_LDFLAGS) -o kernel/chronos.o $(KERNEL_OBJECTS) $(KERNEL_DRIVERS) $(LIBS)

kernel/boot/boot-stage1.img: kernel/boot/ata-read.o
	$(AS) $(ASFLAGS) $(BUILD_ASFLAGS) -I include -c -o kernel/boot/bootasm.o kernel/boot/bootasm.S
	$(LD) $(LDFLAGS) $(BOOT_STAGE1_LDFLAGS) -o kernel/boot/boot-stage1.o kernel/boot/bootasm.o kernel/boot/ata-read.o
	$(OBJCOPY) -S -O binary -j .text kernel/boot/boot-stage1.o kernel/boot/boot-stage1.img

kernel/boot/boot-stage2.img: libs $(KERNEL_DRIVERS)
	$(CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -I include -c -o kernel/boot/bootc.o kernel/boot/bootc.c
	$(LD) $(LDFLAGS) $(BOOT_STAGE2_LDFLAGS) -o kernel/boot/boot-stage2.o kernel/boot/bootc.o $(LIBS) $(KERNEL_DRIVERS)
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

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) $(KERNEL_CFLAGS) $(BUILD_CFLAGS) -c -o $@ $<

kernel/drivers/%.o: kernel/drivers/%.c
	$(CC) $(CFLAGS) $(KERNEL_CFLAGS) $(BUILD_CFLAGS)-c -o $@ $<
