AS=as
LD=ld

BINS := \
	program

.PHONY: all clean

all: $(BINS)

clean:
	rm -f $(BINS) assembly

program:
	$(AS) $(ASFLAGS) program.S -o assembly
	$(LD) assmembly -o program
	rm ./assembly
