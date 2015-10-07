# Use the new tool chain to build executables.
TARGET=i686-pc-chronos-
TOOL_DIR=../tools/bin
CROSS_CC=$(TOOL_DIR)/$(TARGET)gcc
CROSS_LD=$(TOOL_DIR)/$(TARGET)ld
CROSS_AS=$(TOOL_DIR)/$(TARGET)gcc
CROSS_OBJCOPY=$(TOOL_DIR)/$(TARGET)objcopy
TARGET_SYSROOT=../sysroot
USER=$(shell whoami)

# use host to configure the tools
CC=gcc
ld=ld
AS=gcc
OBJCOPY=objcopy

LDFLAGS := 
CFLAGS := -ggdb -Werror -Wall -gdwarf-2 -fno-common
ASFLAGS += -ggdb -Werror -Wall
QEMU := qemu-system-i386

# Flags for building indipendant binaries

# Disable Position Independant Code
# BUILD_CFLAGS += -fno-pic
# Disable built in functions
BUILD_CFLAGS += -fno-builtin
# Disable optimizations that deal with aliases.
# BUILD_CFLAGS += -fno-strict-aliasing
# Disable stack smashing protection
BUILD_CFLAGS += -fno-stack-protector

BUILD_ASFLAGS += $(BUILD_CFLAGS)

# Create a 128MB Hard drive
FS_TYPE := ext2.img
FS_DD_BS := 4096
FS_DD_COUNT := 32768
FS_START := 2048

.PHONY: all
all: chronos.img

include tools/makefile.mk
include kernel/makefile.mk
include user/makefile.mk



chronos.img: kernel/boot/boot-stage1.img \
		kernel/boot/boot-stage2.img \
		$(FS_TYPE)
	dd if=/dev/zero of=chronos.img bs=512 count=2048
	tools/bin/boot-sign kernel/boot/boot-stage1.img
	dd if=kernel/boot/boot-stage1.img of=chronos.img count=1 bs=512 conv=notrunc seek=0
	dd if=kernel/boot/boot-stage2.img of=chronos.img count=$(BOOT_STAGE2_SECTORS) bs=512 conv=notrunc seek=1
	dd if=$(FS_TYPE) of=chronos.img bs=512 conv=notrunc seek=$(FS_START)

virtualbox: tools chronos.img
	./tools/virtualbox.sh

vsfs.img: $(TOOLS_BUILD) kernel/chronos.o $(USER_BUILD)
	mkdir -p fs
	mkdir -p fs/boot
	mkdir -p fs/bin
	cp kernel/chronos.o fs/boot/chronos.elf
	cp -R user/bin/* fs/bin/
	cp -R ../sysroot/* fs/
	./tools/bin/mkfs -i 8192 -s 134217728 -r fs $(FS_TYPE)
#	./tools/bin/mkfs -i 128 -s 16777216 -r fs fs.img

ext2.img: kernel/chronos.o $(USER_BUILD)
	echo "Super user privileges are needed for loop mounting..."
	sudo echo ""
	dd if=/dev/zero of=./ext2.img bs=$(FS_DD_BS) count=$(FS_DD_COUNT) seek=0
# Create the file system
	echo "yes" | mkfs.ext2 ./ext2.img
	rm -rf tmp
	mkdir -p tmp
	sudo mount -o loop ./ext2.img ./tmp
	sudo chown -R $(USER):$(USER) tmp
	cp -R $(TARGET_SYSROOT)/* ./tmp
	mkdir -p ./tmp/bin
	cp -R ./user/bin/* ./tmp/bin/
	mkdir -p ./tmp/boot
	cp ./kernel/chronos.o ./tmp/boot/chronos.elf
	sudo umount ./tmp
	rm -rf tmp

ext2-grub.img:
	echo "Super user privileges are needed for loop mounting..."
	sudo echo ""
	dd if=/dev/zero of=./ext2.img bs=$(FS_DD_BS) count=$(FS_DD_COUNT) seek=0
# Create the partition table
	printf "n\n\n\n\n\nw\n" | fdisk -b 512 ext2.img
# Create the file system
	echo "yes" | mkfs.ext2 -E offset=1048576 ./ext2.img
	rm -rf tmp
	mkdir -p tmp
	sudo mount -o loop,offset=1048576 ./ext2.img ./tmp
	sudo chown -R $(USER):$(USER) tmp
	cp -R $(TARGET_SYSROOT)/* ./tmp
	sudo umount ./tmp
	rm -rf tmp
	sudo grub-install ./ext2.img

fsck: fs.img tools/bin/fsck
	tools/bin/fsck fs.img

kernel/idt.c:
	tools/bin/mkvect > kernel/idt.c

QEMU_CPU_COUNT := -smp 1
QEMU_BOOT_DISK := chronos.img
QEMU_MAX_RAM := -m 512M
# QEMU_NOX := -nographic

QEMU_OPTIONS := $(QEMU_CPU_COUNT) $(QEMU_MAX_RAM) $(QEMU_NOX) $(QEMU_BOOT_DISK)

.PHONY: qemu qemu-gdb qemu-x qemu-x-gdb

qemu: all
	$(QEMU) -nographic $(QEMU_OPTIONS)

qemu-gdb: all kernel-symbols user-symbols
	$(QEMU) -nographic $(QEMU_OPTIONS) -s -S

qemu-x: all 
	$(QEMU) $(QEMU_OPTIONS)

qemu-x-gdb: all kernel-symbols user-symbols
	$(QEMU) $(QEMU_OPTIONS) -s -S

run:
	$(QEMU) -nographic $(QEMU_OPTIONS)
run-x:
	$(QEMU) $(QEMU_OPTIONS)
run-gdb: kernel-symbols user/bin $(USER_BUILD) user-symbols
	$(QEMU) -nographic $(QEMU_OPTIONS) -s -S

soft-clean:
	rm -rf $(USER_CLEAN) $(KERNEL_CLEAN) $(TOOLS_CLEAN)

patch: soft-clean kernel/chronos.o kernel/boot/boot-stage1.img  kernel/boot/boot-stage2.img $(USER_BUILD)
	tools/bin/boot-sign kernel/boot/boot-stage1.img
	dd if=kernel/boot/boot-stage1.img of=chronos.img count=1 bs=512 conv=notrunc seek=0
	dd if=kernel/boot/boot-stage2.img of=chronos.img count=62 bs=512 conv=notrunc seek=1
	./tools/bin/fsck -s 64 chronos.img cp kernel/chronos.o /boot/chronos.elf

.PHONY: clean
clean: 
	rm -rf $(KERNEL_CLEAN) $(TOOLS_CLEAN) $(LIBS_CLEAN) $(USER_CLEAN) fs fs.img chronos.img $(USER_LIB_CLEAN) .bochsrc bochsout.txt chronos.vdi tmp ext2.img
