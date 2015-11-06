# Specify build targets. Exclude the file extension (e.g. .c or .s)
KERNEL_OBJECTS := \
	arch/$(BUILD_ARCH)/vm/vm \
	arch/$(BUILD_ARCH)/vm/vm_alloc \
	arch/$(BUILD_ARCH)/vm/pgdir \
	cache/diskcache \
	cache/cacheman \
	cache/cache \
	netman \
	main \
	proc \
	tty \
	panic \
	kcond \
	syscall/syscall \
	syscall/sysnet \
	syscall/syssig \
	syscall/sysproc \
	syscall/sysfile \
	syscall/sysutil \
	syscall/sysmmap \
	trap \
	cpu \
	pipe \
	fsman \
	devman \
	iosched \
	ktime \
	signal \
	stdlock \
	file \
	stdlib \
	ioctl

# assembly files
KERNEL_ASSEMBLY := \
	arch/$(BUILD_ARCH)/vm/asm \
	asm \
	idt \
	boot/bootc_jmp

# Specify driver files. Exclude all file extensions
KERNEL_DRIVERS := \
	ata \
	keyboard \
	pic \
	pit \
	serial \
	console \
        ext2 \
	ramfs \
	cmos \
	rtc \
	diskio \
	raid

#        vsfs 

# Add kernel/ before all of the kernel targets
KERNEL_OBJECTS := $(addprefix kernel/, $(KERNEL_OBJECTS))
KERNEL_OBJECTS := $(addsuffix .o, $(KERNEL_OBJECTS))

# assembly objects need to end in .o and start with kernel/
KERNEL_ASSEMBLY_OBJECTS := $(addprefix kernel/, $(KERNEL_ASSEMBLY))
KERNEL_ASSEMBLY_OBJECTS := $(addsuffix .o, $(KERNEL_ASSEMBLY_OBJECTS))

# put drivers into the form kernel/drivers/file.o
KERNEL_DRIVERS := $(addprefix kernel/drivers/, $(KERNEL_DRIVERS))
KERNEL_DRIVERS := $(addsuffix .o, $(KERNEL_DRIVERS))

# Specify files to clean
KERNEL_CLEAN := \
        $(KERNEL_OBJECTS) \
        $(KERNEL_DRIVERS) \
	$(KERNEL_ASSEMBLY_OBJECTS) \
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
	kernel/boot/ext2.o \
	kernel/boot/cache.o \
	kernel/idt.S \
	ata-read.sym \
	boot-stage1.sym \
	boot-stage2.sym \
	chronos.sym

# Include files
KERNEL_CFLAGS += -I kernel/include
# Include driver headers
KERNEL_CFLAGS += -I kernel/drivers
# Include kernel headers
KERNEL_CFLAGS += -I kernel
# Include cache headers
KERNEL_CFLAGS += -I kernel/cache

# Don't link the standard library
KERNEL_LDFLAGS := -nostdlib
# Set the entry point
KERNEL_LDFLAGS += --entry=main
# Set the memory location where the kernel will be placed
KERNEL_LDFLAGS += --section-start=.text=0xFF000000
# KERNEL_LDFLAGS += --omagic

BOOT_STAGE1_LDFLAGS := --section-start=.text=0x7c00 --entry=start

BOOT_STAGE2_CFLAGS  := -I kernel/drivers \
		-I kernel/include \
		-I kernel/cache \
		-I kernel

BOOT_STAGE2_LDFLAGS := --section-start=.text=0x7E00 --entry=main \
                --section-start=.data=0x17E00 \
                --section-start=.rodata=0x18600 \
                --section-start=.bss=0x18e00

# These should reflect the above sector counts
BOOT_STAGE2_TEXT_START := 0
BOOT_STAGE2_TEXT_SZ := 128

# How many sectors do our data sections get?
BOOT_STAGE2_SECT_COUNT := 4

# Where do each of the simple sections start on disk?
BOOT_STAGE2_DATA_START := 128
BOOT_STAGE2_RODATA_START := 132
BOOT_STAGE2_BSS_START := 136

# How many sectors does stage 2 take up?
BOOT_STAGE2_SECTORS := 140

# What should be linked into the second stage boot loader?
# NOTE: keep this list minimal!
BOOT_STAGE2_DEPS := kernel/boot/bootc.o kernel/boot/bootc_jmp.o kernel/arch/$(BUILD_ARCH)/vm/vm_alloc.o kernel/arch/$(BUILD_ARCH)/vm/asm.o kernel/cpu.o kernel/arch/$(BUILD_ARCH)/vm/pgdir.o kernel/boot/ext2.o kernel/drivers/serial.o kernel/drivers/ata.o kernel/stdlock.o kernel/drivers/pic.o kernel/file.o kernel/stdlib.o kernel/boot/cache.o kernel/drivers/diskio.o kernel/cache/diskcache.o kernel/cache/cacheman.o


.PHONY: kernel-symbols
kernel-symbols: kernel/chronos.o kernel/boot/ata-read.o
	$(CROSS_OBJCOPY) --only-keep-debug kernel/chronos.o chronos.sym
	$(CROSS_OBJCOPY) --only-keep-debug kernel/boot/boot-stage1.o boot-stage1.sym
	$(CROSS_OBJCOPY) --only-keep-debug kernel/boot/boot-stage2.o boot-stage2.sym
	$(CROSS_OBJCOPY) --only-keep-debug kernel/boot/ata-read.o ata-read.sym


kernel/chronos.o: kernel/idt.o $(KERNEL_OBJECTS) $(KERNEL_DRIVERS) $(KERNEL_ASSEMBLY_OBJECTS) kernel/idt.S $(KERNEL_ASSEMBLY_OBJECTS)
	$(CROSS_LD) $(LDFLAGS) $(KERNEL_LDFLAGS) -o kernel/chronos.o $(KERNEL_OBJECTS) $(KERNEL_DRIVERS) $(KERNEL_ASSEMBLY_OBJECTS)

kernel/boot/boot-stage1.img: kernel/boot/ata-read.o
	$(CROSS_AS) $(ASFLAGS) $(BUILD_ASFLAGS) -I kernel/include -c -o kernel/boot/bootasm.o kernel/boot/bootasm.S
	$(CROSS_LD) $(LDFLAGS) $(BOOT_STAGE1_LDFLAGS) -o kernel/boot/boot-stage1.o kernel/boot/bootasm.o kernel/boot/ata-read.o
	$(CROSS_OBJCOPY) -S -O binary -j .text kernel/boot/boot-stage1.o kernel/boot/boot-stage1.img

kernel/boot/boot-stage2.img: $(KERNEL_DRIVERS) $(TOOLS_BUILD) $(KERNEL_OBJECTS) $(KERNEL_ASSEMBLY_OBJECTS)
	# Build special files for boot stage 2
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -D__BOOT_2__ -c -o kernel/boot/ext2.o kernel/drivers/ext2.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -D__BOOT_2__ -c -o kernel/boot/cache.o kernel/cache/cache.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -c -o kernel/boot/bootc.o kernel/boot/bootc.c
	$(CROSS_LD) $(LDFLAGS) $(BOOT_STAGE2_LDFLAGS) -o kernel/boot/boot-stage2.o $(BOOT_STAGE2_DEPS)
	$(CROSS_OBJCOPY) -O binary -j .text kernel/boot/boot-stage2.o kernel/boot/boot-stage2.text	
	$(CROSS_OBJCOPY) -O binary -j .data kernel/boot/boot-stage2.o kernel/boot/boot-stage2.data
	$(CROSS_OBJCOPY) -O binary -j .rodata kernel/boot/boot-stage2.o kernel/boot/boot-stage2.rodata	
	$(CROSS_OBJCOPY) -O binary -j .bss kernel/boot/boot-stage2.o kernel/boot/boot-stage2.bss
	./tools/bin/boot2-verify
	dd if=kernel/boot/boot-stage2.text of=kernel/boot/boot-stage2.img bs=512 count=$(BOOT_STAGE2_TEXT_SZ) seek=0
	dd if=kernel/boot/boot-stage2.data of=kernel/boot/boot-stage2.img bs=512 count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_DATA_START)
	dd if=kernel/boot/boot-stage2.rodata of=kernel/boot/boot-stage2.img bs=512 count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_RODATA_START)
	dd if=kernel/boot/boot-stage2.bss of=kernel/boot/boot-stage2.img bs=512 count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_BSS_START)

kernel/boot/ata-read.o:
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) -I kernel/include -Os -c -o kernel/boot/ata-read.o kernel/boot/ata-read.c

kernel/idt.S: tools/bin/mkvect
	tools/bin/mkvect > kernel/idt.S
kernel/%.o: kernel/%.S
	$(CROSS_AS) $(ASFLAGS) $(BUILD_ASFLAGS) -I kernel/include -I kernel/ -c -o $@ $<

kernel/%.o: kernel/%.c
	$(CROSS_CC) $(CFLAGS) $(KERNEL_CFLAGS) $(BUILD_CFLAGS) -c -o $@ $<

kernel/drivers/%.o: kernel/drivers/%.c
	$(CROSS_CC) $(CFLAGS) $(KERNEL_CFLAGS) $(BUILD_CFLAGS) -c -o $@ $<
