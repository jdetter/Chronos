BOOT_STAGE1_DEPS := \
	stage1/stage1 \
	stage1/ata
BOOT_STAGE1_DEPS := $(addsuffix .o, $(BOOT_STAGE1_DEPS))

BOOT_STAGE2_DEPS := \
	stage2/stage2 \
	stage2/stage2_jmp \
	arch/stdlock \
	arch/vm/asm \
	arch/vm/pgdir \
	arch/vm/vm_alloc \
	arch/vm/vm_boot \
	arch/drivers/serial \
	arch/drivers/ata \
	arch/drivers/pic \
	arch/x86/asm \
	arch/cpu \
	kern/drivers/diskio \
	kern/drivers/ext2 \
	kern/cache/cache \
	kern/cache/diskcache \
	kern/cache/cacheman \
	kern/stdlock \
	kern/file \
	kern/stdlib
BOOT_STAGE2_DEPS := $(addsuffix .o, $(BOOT_STAGE2_DEPS))

MULTIBOOT_DEPS := \
	multiboot/multiboot \
	multiboot/multiboot_header
MULTIBOOT_DEPS := $(addsuffix .o, $(MULTIBOOT_DEPS))

BOOT_INCLUDE := \
	-Ikernel-include \
	-Iarch-include

# Stage 1 configuration
BOOT_STAGE1_LDFLAGS := --section-start=.text=0x7c00 --entry=start

# Stage 2 configuration
BOOT_STAGE2_LDFLAGS := \
	--entry=main \
	--section-start=.text=0x7E00 \
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

BOOT_CLEAN := \
	boot-stage2.bss \
	boot-stage2.data \
	boot-stage2.rodata \
	boot-stage2.text

.PHONY: boot-clean
boot-clean:
	rm -f  \
		boot-stage1.o boot-stage1.img \
		boot-stage2.o boot-stage2.img $(BOOT_CLEAN) \
		$(BOOT_STAGE2_DEPS) $(BOOT_STAGE1_DEPS) $(MULTIBOOT_DEPS)

boot-stage1.img: $(BOOT_STAGE1_DEPS)
	$(CROSS_LD) $(LDFLAGS) $(BOOT_STAGE1_LDFLAGS) -o ./boot-stage1.o $(BOOT_STAGE1_DEPS)
	$(CROSS_OBJCOPY) -S -O binary -j .text boot-stage1.o boot-stage1.img

boot-stage2.img: $(BOOT_STAGE2_DEPS)
	# Link the second boot stage 
	$(CROSS_LD) $(LDFLAGS) $(BOOT_STAGE2_LDFLAGS) -o boot-stage2.o $(BOOT_STAGE2_DEPS)
	# Extract the program parts
	$(CROSS_OBJCOPY) -O binary -j .text boot-stage2.o boot-stage2.text
	$(CROSS_OBJCOPY) -O binary -j .data boot-stage2.o boot-stage2.data
	$(CROSS_OBJCOPY) -O binary -j .rodata boot-stage2.o boot-stage2.rodata
	$(CROSS_OBJCOPY) -O binary -j .bss boot-stage2.o boot-stage2.bss
	../tools/bin/boot2-verify
	dd if=boot-stage2.text of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_TEXT_SZ) seek=0
	dd if=boot-stage2.data of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_DATA_START)
	dd if=boot-stage2.rodata of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_RODATA_START)
	dd if=boot-stage2.bss of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_BSS_START)

multiboot.o: $(MULTIBOOT_DEPS) $(BOOT_STAGE2_DEPS)
	$(CROSS_CC) $(CFLAGS) $(BUILD_CLAGS) $(BOOT_INCLUDE) $(i386_INCLUDE) -o multiboot.o $(MULTIBOOT_DEPS) $(BOOT_STAGE2_DEPS)

%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(BUILD_AFLAGS) $(BOOT_INCLUDE) -D__BOOT_STRAP__ -c -o $@ $<

%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_INCLUDE) -c -o $@ $<
