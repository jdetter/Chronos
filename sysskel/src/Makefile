AS=as
LD=ld
CC=gcc

BINS := \
	program

LIBS := \
	sys_write

.PHONY: all clean

all: $(BINS)

lib:
	$(AS) -o lib sys_write.S

cprogram: lib
	$(CC) -c -o compiled ./cprog.c
	$(LD) -o program ./compiled ./lib

clean:
	rm -f $(BINS) assembly

program:
	$(AS) $(ASFLAGS) program.S -o assembly
	$(LD) assembly -o program
	rm ./assembly
