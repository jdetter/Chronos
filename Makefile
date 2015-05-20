CC=gcc
LD=ld

LDFLAGS := -m elf_i386
CFLAGS := -ggdb -m32

.PHONY: all
all: libs tools kernel/chronos.img

.PHONY: clean
clean: kernel-clean user-clean tools-clean lib-clean li_clean

include kernel/makefile.mk
include user/makefile.mk
include tools/makefile.mk
include lib/makefile.mk
include linux/makefile.mk

.DEFAULT: all
