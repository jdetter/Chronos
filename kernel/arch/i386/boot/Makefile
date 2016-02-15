# Stage 1 configuration
BOOT_STAGE1_LDFLAGS := --section-start=.text=0x7c00 --entry=start

# Stage 2 configuration
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
i386_INCLUDE := $(addprefix ../, $(i386_INCLUDE))

boot-stage2.img:
	cd deps ; \
	make CROSS_CC="$(CROSS_CC)" \
		CROSS_LD="$(CROSS_LD)" \
		CROSS_AS="$(CROSS_AS)" \
		AFLAGS="$(AFLAGS)" \
		CFLAGS="$(CFLAGS)" \
		BUILD_AFLAGS="$(BUILD_AFLAGS)" \
		BUILD_CFLAGS="$(BUILD_CFLAGS)" \
		i386_INCLUDE="$(i386_INCLUDE)" \
		stage2-deps.o

	exit 1
	# Build special files for boot stage 2
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -D__BOOT_STRAP__ -c -o kernel/boot/bootc.o kernel/boot/bootc.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -D__BOOT_STRAP__ -c -o kernel/boot/bootc_jmp.o kernel/boot/bootc_jmp.S
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -D__BOOT_STRAP__ -c -o kernel/boot/ext2.o kernel/drivers/ext2.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -D__BOOT_STRAP__ -c -o kernel/boot/cache.o kernel/cache/cache.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -D__BOOT_STRAP__ -c -o kernel/boot/vm_alloc.o \
		kernel/arch/$(BUILD_ARCH)/vm/vm_alloc.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) -D__BOOT_STRAP__ -c -o kernel/boot/pgdir.o \
		kernel/arch/$(BUILD_ARCH)/vm/pgdir.c
	$(CROSS_LD) $(LDFLAGS) $(BOOT_STAGE2_LDFLAGS) -o kernel/boot/boot-stage2.o $(BOOT_STAGE2_SPECIAL) $(BOOT_STAGE2_DEPS)
	$(CROSS_OBJCOPY) -O binary -j .text kernel/boot/boot-stage2.o kernel/boot/boot-stage2.text	
	$(CROSS_OBJCOPY) -O binary -j .data kernel/boot/boot-stage2.o kernel/boot/boot-stage2.data
	$(CROSS_OBJCOPY) -O binary -j .rodata kernel/boot/boot-stage2.o kernel/boot/boot-stage2.rodata	
	$(CROSS_OBJCOPY) -O binary -j .bss kernel/boot/boot-stage2.o kernel/boot/boot-stage2.bss
	./tools/bin/boot2-verify
	dd if=kernel/boot/boot-stage2.text of=kernel/boot/boot-stage2.img bs=512 count=$(BOOT_STAGE2_TEXT_SZ) seek=0
	dd if=kernel/boot/boot-stage2.data of=kernel/boot/boot-stage2.img bs=512 count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_DATA_START)
	dd if=kernel/boot/boot-stage2.rodata of=kernel/boot/boot-stage2.img bs=512 count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_RODATA_START)
	dd if=kernel/boot/boot-stage2.bss of=kernel/boot/boot-stage2.img bs=512 count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_BSS_START)
