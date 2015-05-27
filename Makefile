CC=gcc
LD=ld
AS=gcc
OBJCOPY=objcopy

LDFLAGS := -m elf_i386 --omagic
CFLAGS := -ggdb -m32 -Werror -Wall
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
all: tools chronos.img lib-tests

virtualbox: tools chronos.img
	./tools/virtualbox.sh

QEMU_CPU_COUNT := -smp 1
QEMU_BOOT_DISK := chronos.img
QEMU_MAX_RAM := -m 512M
# QEMU_NOX := -nographic

QEMU_OPTIONS := $(QEMU_CPU_COUNT) $(QEMU_MAX_RAM) $(QEMU_NOX) $(QEMU_BOOT_DISK)

qemu: all
	$(QEMU) -nographic $(QEMU_OPTIONS)

qemu-gdb: all kernel-symbols
	$(QEMU) -nographic $(QEMU_OPTIONS) -s -S

qemu-x: all 
	$(QEMU) $(QEMU_OPTIONS)

include kernel/makefile.mk
include user/makefile.mk
include tools/makefile.mk
include lib/makefile.mk
include linux/makefile.mk

.PHONY: clean
clean: 
	rm -rf $(KERNEL_CLEAN) $(TOOLS_CLEAN) $(LIBS_CLEAN) $(LINUX_CLEAN)

.DEFAULT: all
