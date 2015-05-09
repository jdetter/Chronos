include user/makefile.mk
include tools/makefile.mk

CC=gcc
LD=ld

LDFLAGS := -m elf_i386
CFLAGS := -ggdb -m32

.PHONY: all
all: tools user-test

clean: user-clean tools-clean
