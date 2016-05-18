ifndef CROSS_CC

TARGET=i686-pc-chronos-
export BUILD_ARCH := i386
TOOL_DIR=/srv/detter/i386/toolchain/tools/bin


CROSS_CC := $(TOOL_DIR)/$(TARGET)gcc
export CROSS_CC := $(shell readlink -e "$(CROSS_CC)")
CROSS_LD := $(TOOL_DIR)/$(TARGET)ld
export CROSS_LD := $(shell readlink -e $(CROSS_LD))
CROSS_AS := $(TOOL_DIR)/$(TARGET)gcc
export CROSS_AS := $(shell readlink -e $(CROSS_AS))
CROSS_OBJCOPY := $(TOOL_DIR)/$(TARGET)objcopy
export CROSS_OBJCOPY := $(shell readlink -e $(CROSS_OBJCOPY))
TARGET_SYSROOT := /srv/detter/i386/sysroot
export TARGET_SYSROOT := $(shell readlink -e $(TARGET_SYSROOT))
export USER := $(shell whoami)


# use host to configure the tools
export CC=gcc
export LD=ld
export AS=gcc
export OBJCOPY=objcopy

export LDFLAGS :=
export CFLAGS := -ggdb -Werror -Wall -gdwarf-2 -fno-common -DARCH_$(BUILD_ARCH) -DARCH_STR=$(BUILD_ARCH) -fno-builtin -fno-stack-protector $(CFLAGS)
export AFLAGS := -ggdb -Werror -Wall -DARCH_$(BUILD_ARCH) -DARCH_STR=$(BUILD_ARCH) $(AFLAGS)
QEMU := qemu-system-$(BUILD_ARCH)


.PHONY: all clean
all: multiboot.o
clean: boot-clean

endif

BOOT_STAGE1_DEPS := \
        stage1/ata
BOOT_STAGE1_DEPS := $(addsuffix .o, $(BOOT_STAGE1_DEPS))

BOOT_STAGE2_DEPS := \
        arch-vm/asm \
        arch-vm/pgdir \
        arch-vm/vm_alloc \
        arch-drivers/serial \
        arch-drivers/ata \
        arch-drivers/pic \
        kern-drivers/diskio \
        kern-drivers/ext2 \
        cache/cache \
        cache/diskcache \
        cache/cacheman \
        kern/cpu \
        kern/stdlock \
        kern/file \
        kern/stdlib
BOOT_STAGE2_DEPS := $(addsuffix .o, $(BOOT_STAGE2_DEPS))

BOOT_DEPS_INCLUDE := \
	-Iinclude

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

BOOT_OBJS := \
	stage1 \
	stage2 \
	stage2_jmp
BOOT_OBJS := $(addsuffix .o, $(BOOT_OBJS))

BOOT_CLEAN := \
	boot-stage2.bss \
	boot-stage2.data \
	boot-stage2.rodata \
	boot-stage2.text

.PHONY: boot-clean
boot-clean:
	cd deps ; \
	make deps-clean 
	rm -f  \
		boot-stage1.o boot-stage1.img \
		boot-stage2.o boot-stage2.img \
		$(BOOT_OBJS) $(BOOT_CLEAN)
		

boot-stage1.img: $(BOOT_OBJS) $(BOOT_STAGE1_DEPS)
	cd deps; \
	make stage1-deps.o

	$(CROSS_LD) $(LDFLAGS) $(BOOT_STAGE1_LDFLAGS) -o ./boot-stage1.o stage1.o ./deps/stage1-deps.o
	$(CROSS_OBJCOPY) -S -O binary -j .text boot-stage1.o boot-stage1.img
	# Sign the boot sector
	../../../../tools/bin/boot-sign ./boot-stage1.img

boot-stage2.img: $(BOOT_OBJS) $(BOOT_STAGE2_DEPS)
	cd deps ; \
	make stage2-deps.o

	# Link the second boot stage 
	$(CROSS_LD) $(LDFLAGS) $(BOOT_STAGE2_LDFLAGS) \
		-o boot-stage2.o stage2.o stage2_jmp.o ./deps/stage2-deps.o

	# Extract the program parts
	$(CROSS_OBJCOPY) -O binary -j .text boot-stage2.o boot-stage2.text
	$(CROSS_OBJCOPY) -O binary -j .data boot-stage2.o boot-stage2.data
	$(CROSS_OBJCOPY) -O binary -j .rodata boot-stage2.o boot-stage2.rodata
	$(CROSS_OBJCOPY) -O binary -j .bss boot-stage2.o boot-stage2.bss
	../../../../tools/bin/boot2-verify
	dd if=boot-stage2.text of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_TEXT_SZ) seek=0
	dd if=boot-stage2.data of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_DATA_START)
	dd if=boot-stage2.rodata of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_RODATA_START)
	dd if=boot-stage2.bss of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_BSS_START)

multiboot.o: $(BOOT_OBJS)

%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_DEPS_INCLUDE) $(i386_INCLUDE) -D__BOOT_STRAP__ -c -o $@ $< 
%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(BUILD_AFLAGS) $(BOOT_DEPS_INCLUDE) $(i386_INCLUDE) -D__BOOT_STRAP__ -c -o $@ $<
