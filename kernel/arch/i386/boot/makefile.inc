d := $(dir)

BOOT_BUILD_DIR := $(ARCH_BUILD_DIR)$(d)

BOOT_INCLUDE := $(ARCH_INCLUDE)

BOOT_STAGE1_KERNEL_OBJS :=\
	drivers/ata

BOOT_STAGE1_BOOT_OBJS :=\
	stage1/stage1 \

BOOT_STAGE1_BOOT_OBJS := $(addprefix $(d), $(BOOT_STAGE1_BOOT_OBJS))
BOOT_STAGE1_KERNEL_OBJS := $(addprefix $(KERNEL_DIR), $(BOOT_STAGE1_KERNEL_OBJS))

BOOT_STAGE1_OBJS += $(BOOT_STAGE1_KERNEL_OBJS ) $(BOOT_STAGE1_BOOT_OBJS)
BOOT_STAGE1_OBJS := $(addprefix $(ARCH_BUILD_DIR), $(addsuffix .o, $(BOOT_STAGE1_OBJS)))


BOOT_STAGE2_BOOT_OBJS := \
	stage2/stage2 \
	stage2/stage2_jmp \
	stage2/vm \

BOOT_STAGE2_KERNEL_OBJS :=\
	drivers/storageio \
	drivers/ext2 \
	cache/cache \
	cache/storagecache \
	cache/cacheman \
	stdlock \
	file \
	stdlib

BOOT_STAGE2_ARCH_OBJS :=\
	vm/asm \
	vm/pgdir \
	vm/vm_alloc \
	drivers/serial \
	drivers/ata \
	drivers/pic \
	x86/asm

BOOT_STAGE2_BOOT_OBJS := $(addprefix $(d), $(BOOT_STAGE2_BOOT_OBJS))
BOOT_STAGE2_KERNEL_OBJS := $(addprefix $(KERNEL_DIR), $(BOOT_STAGE2_KERNEL_OBJS))
BOOT_STAGE2_ARCH_OBJS := $(addprefix $(ARCH_DIR), $(BOOT_STAGE2_ARCH_OBJS))

BOOT_STAGE2_OBJS += $(BOOT_STAGE2_KERNEL_OBJS) $(BOOT_STAGE2_ARCH_OBJS) $(BOOT_STAGE2_BOOT_OBJS)
BOOT_STAGE2_OBJS := $(addprefix $(ARCH_BUILD_DIR), $(addsuffix .o, $(BOOT_STAGE2_OBJS)))


MULTIBOOT_BOOT_OBJS := \
	multiboot/multiboot \
	multiboot/multiboot_header

MULTIBOOT_BOOT_OBJS := $(addprefix $(d),$(MULTIBOOT_BOOT_OBJS))
MULTIBOOT_OBJS += $(MULTIBOOT_BOOT_OBJS)
MULTIBOOT_OBJS := $(addprefix $(ARCH_BUILD_DIR), $(addsuffix .o, $(MULTIBOOT_OBJS)))


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
BOOT_STAGE2_DATA_SZ := 4
# Where do each of the simple sections start on disk?
BOOT_STAGE2_DATA_START := 128
BOOT_STAGE2_RODATA_START := 132
BOOT_STAGE2_BSS_START := 136
# How many sectors does stage 2 take up?
BOOT_STAGE2_SECTORS := 140


$(BUILD_DIR)boot-stage1.img: $(BOOT_STAGE1_OBJS)
	$(CROSS_LD) $(LDFLAGS) $(BOOT_STAGE1_LDFLAGS) -o ./boot-stage1.o $(BOOT_STAGE1_OBJS)
	$(CROSS_OBJCOPY) -S -O binary -j .text boot-stage1.o $@

#TODO: FIXME
$(BUILD_DIR)boot-stage2.img: tools $(BOOT_STAGE2_OBJS)
	@echo $(BOOT2_IMAGER) -o $@ $(BOOT_STAGE2_OBJS)

$(BUILD_DIR)multiboot.o: $(MULTIBOOT_OBJS) $(BOOT_STAGE2_OBJS)
	$(CROSS_CC) $(CFLAGS) $(BUILD_CLAGS) $(BOOT_INCLUDE) $(i386_INCLUDE) -o $@ $(MULTIBOOT_OBJS) $(BOOT_STAGE2_OBJS)

# Dir specific builds
$(BOOT_BUILD_DIR)%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(BUILD_AFLAGS) $(BOOT_INCLUDE) -D__BOOT_STRAP__ -c -o $@ $<

$(BOOT_BUILD_DIR)%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_INCLUDE) -c -o $@ $<

BOOT_CLEAN += boot-stage1.o boot-stage1.img \
		 boot-stage2.o boot-stage2.img \
		 $(BOOT_STAGE2_OBJS) $(BOOT_STAGE1_OBJS) $(MULTIBOOT_OBJS) \
		 boot-stage2.o.bss \
		 boot-stage2.o.data \
		 boot-stage2.o.rodata \
		 boot-stage2.o.text

PHONY += boot-clean
boot-clean:
	rm -f $(BOOT_CLEAN)


CLEAN += $(BOOT_CLEAN)

#TODO: Add -include on deps

