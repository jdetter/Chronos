AS=as
LD=ld

BINS := \
	program

.PHONY: all clean

all: $(BINS)

clean:
	rm -f $(BINS)

program:
	$(AS) $(ASFLAGS) program.S
	$(LD) a.out -o program
	rm ./a.out
