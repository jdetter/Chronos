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

QEMU_CPU_COUNT := 1
QEMU_BOOT_DISK := chronos.img
QEMU_MAX_RAM := 512M
QEMU_SERIAL := -serial mon:stdin
QEMU_NOX := -nographic

qemu: all
	$(QEMU) -m $(QEMU_MAX_RAM) $(QEMU_BOOT_DISK) -smp $(QEMU_CPU_COUNT)	

qemu-gdb: all kernel-symbols

test:
	$(QEMU) -m $(QEMU_MAX_RAM) $(QEMU_BOOT_DISK) -smp $(QEMU_CPU_COUNT) -s -S

include kernel/makefile.mk
include user/makefile.mk
include tools/makefile.mk
include lib/makefile.mk
include linux/makefile.mk
include include/makefile.mk

.PHONY: clean
clean: 
	rm -rf $(KERNEL_CLEAN) $(INCLUDE_CLEAN) $(TOOLS_CLEAN) $(LIBS_CLEAN) $(LINUX_CLEAN)

.DEFAULT: all
