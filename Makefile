CC=gcc
LD=ld

LDFLAGS := -m elf_i386
CFLAGS := -ggdb -m32

# Flags for building indipendant binaries

# Disable Position Independant Code
BUILD_CFLAGS += -fno-pic
# Disable built in functions
BUILD_CFLAGS += -fno-builtin
# Disable optimizations that deal with aliases.
BUILD_CFLAGS += -fno-strict-aliasing
# Disable stack smashing protection
BUILD_CFLAGS += -fno-stack-protector

.PHONY: all
all: tools kernel/chronos.img lib-tests

.PHONY: clean
clean: include-clean kernel-clean user-clean tools-clean lib-clean li_clean

include kernel/makefile.mk
include user/makefile.mk
include tools/makefile.mk
include lib/makefile.mk
include linux/makefile.mk
include include/makefile.mk

.DEFAULT: all
