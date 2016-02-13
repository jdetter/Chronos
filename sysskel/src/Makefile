AS=as

BINS := \
	program

.PHONY: all clean

all: $(BINS)

clean:
	rm -f $(BINS)

program:
	$(AS) $(ASFLAGS) program.S -o program
