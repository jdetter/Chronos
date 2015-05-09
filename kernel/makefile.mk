# Specify build targets. Exclude the file extension (e.g. .c or .s)
KERNEL_TARGETS := \
	main

# Add user/ before all of the program names
KERNEL_TARGETS := $(addprefix kernel/, $(KERNEL_TARGETS))
# Create targets
KERNEL_OBJECTS := $(addsuffix .o, $(KERNEL_TARGETS))
# Source files
KERNEL_SOURCE := $(addsuffix .c, $(KERNEL_TARGETS))

# Include files
KERNEL_CFLAGS += -I include
# Disable Position Independant Code
KERNEL_CFLAGS += -fno-pic
# Disable built in functions
KERNEL_CFLAGS += -fno-builtin
# Disable optimizations that deal with aliases.
KERNEL_CFLAGS += -fno-strict-aliasing
# Disable stack smashing protection
KERNEL_CFLAGS += -fno-stack-protector

# Don't link the standard library
KERNEL_LDFLAGS := -nostdlib
# Set the entry point
KERNEL_LDFLAGS += --entry=main
# Set the memory location where the code should begin
KERNEL_LDFLAGS += --section-start=.text=0x100000
# KERNEL_LDFLAGS += --omagic

kernel/chronos.img: $(KERNEL_OBJECTS)
	@echo $(KERNEL_OBJECTS)
	$(LD) $(LDFLAGS) $(KERNEL_LDFLAGS) -o kernel/chronos.o $(KERNEL_OBJECTS)

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) $(KERNEL_CFLAGS) -c -o $@ $<

kernel-clean:
	rm -f $(KERNEL_OBJECTS)
	rm -f kernel/chronos.o
	rm -f kernel/chronos.img
