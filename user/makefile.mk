include user/lib/makefile.mk

# Specify build targets. Exclude the file extension (e.g. .c or .s)
USER_TARGETS := \
	init \
	example \
	otherexample \
	cat \
	echo \
	ls \
	login \
	mkdir \
	mv \
	rm \
	rmdir \
	sh \
	test \
	stress \
	touch \
	pwd \
	stall \
	stat \
	crash \
	less

# Binary files
USER_BINARIES := $(addprefix user/bin/, $(USER_TARGETS))
# Add user/ before all of the program names
USER_TARGETS := $(addprefix user/, $(USER_TARGETS))
# Create targets
USER_OBJECTS := $(addsuffix .o, $(USER_TARGETS))
# Source files
USER_SOURCE := $(addsuffix .c, $(USER_TARGETS))

# Symbol Files
USER_SYMBOLS := $(addsuffix .sym, $(USER_BINARIES))

# Specify clean targets
USER_CLEAN := \
	user/bin \
	user/syscall.o \
	$(USER_OBJECTS) \
	$(USER_SYMBOLS)

# Include files
USER_CFLAGS += -I include
USER_CFLAGS += -I user/lib
# Disable Position Independant Code
USER_CFLAGS += -fno-pic
# Disable built in functions
USER_CFLAGS += -fno-builtin
# Disable optimizations that deal with aliases.
USER_CFLAGS += -fno-strict-aliasing
# Disable stack smashing protection
USER_CFLAGS += -fno-stack-protector

# Don't link the standard library
USER_LDFLAGS := -nostdlib
# Set the entry point
USER_LDFLAGS += --entry=main
# Set the memory location where the code should begin
USER_LDFLAGS += --section-start=.text=0x1000
# USER_LDFLAGS += --omagic

USER_BUILD :=  $(USER_LIB_OBJECTS) $(LIBS) user/syscall.o user/bin $(USER_OBJECTS) $(USER_BINARIES) 
	
user/bin: 
	mkdir -p user/bin	

user-symbols: $(USER_SYMBOLS)

user/syscall.o:
	$(AS) $(ASFLAGS) $(BUILD_ASFLAGS) -I include  -c user/syscall.S -o user/syscall.o

# Recipe for object files
user/%.o: user/%.c
	$(CC) $(CFLAGS) $(USER_CFLAGS) -c $< -o $@
# Recipe for binary files
user/bin/%: user/%.o
	$(LD) $(LDFLAGS) $(USER_LDFLAGS) -o $@ $< $(LIBS) user/syscall.o $(USER_LIB_OBJECTS)

# Recipe for symbole files
user/bin/%.sym: user/bin/%
	$(OBJCOPY) --only-keep-debug $< $@
