CC=gcc
LD=ld
AS=gcc
OBJCOPY=objcopy

LDFLAGS := -m elf_i386 --omagic
CFLAGS := -ggdb -m32 -Werror -Wall -gdwarf-2 -fno-common
ASFLAGS += -ggdb -m32 -Werror -Wall
QEMU := qemu-system-i386

# Flags for building indipendant binaries

# Disable Position Independant Code
BUILD_CFLAGS += -fno-pic
# Disable built in functions
BUILD_CFLAGS += -fno-builtin
# Disable optimizations that deal with aliases.
BUILD_CFLAGS += -fno-strict-aliasing
# Disable stack smashing protection
BUILD_CFLAGS += -fno-stack-protector

BUILD_ASFLAGS += $(BUILD_CFLAGS)

.PHONY: all
all: chronos.img

include tools/makefile.mk
include lib/makefile.mk
include kernel/makefile.mk
include user/makefile.mk
include user/lib/makefile.mk

chronos.img: kernel/boot/boot-stage1.img \
		kernel/boot/boot-stage2.img \
		fs.img
	dd if=/dev/zero of=chronos.img bs=512 count=2048
	tools/bin/boot-sign kernel/boot/boot-stage1.img
	dd if=kernel/boot/boot-stage1.img of=chronos.img count=1 bs=512 conv=notrunc seek=0
	dd if=kernel/boot/boot-stage2.img of=chronos.img count=62 bs=512 conv=notrunc seek=1
	dd if=fs.img of=chronos.img bs=512 conv=notrunc seek=64

virtualbox: tools chronos.img
	./tools/virtualbox.sh

fs.img: $(TOOLS_BUILD) kernel/chronos.o $(USER_BUILD)
	mkdir -p fs
	mkdir -p fs/boot
	mkdir -p fs/bin
	cp kernel/chronos.o fs/boot/chronos.elf
	cp -R user/bin/* fs/bin/
	./tools/bin/mkfs -i 256 -s 1048576 -r fs fs.img

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

.PHONY: clean
clean: 
	rm -rf $(KERNEL_CLEAN) $(TOOLS_CLEAN) $(LIBS_CLEAN) $(USER_CLEAN) fs fs.img chronos.img $(USER_LIB_CLEAN) .bochsrc bochsout.txt chronos.vdi
