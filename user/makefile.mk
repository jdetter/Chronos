# Specify build targets. Exclude the file extension (e.g. .c or .s)
USER_TARGETS := \
	example \
	otherexample

# Are you debugging?
DEBUG = 1

# Binary files
USER_BINARIES := $(addprefix user/bin/, $(USER_TARGETS))
# Add user/ before all of the program names
USER_TARGETS := $(addprefix user/, $(USER_TARGETS))
# Create targets
USER_OBJECTS := $(addsuffix .o, $(USER_TARGETS))
# Source files
USER_SOURCE := $(addsuffix .c, $(USER_TARGETS))

# Specify clean targets
USER_CLEAN := \
	user/bin \
	$(USER_OBJECTS)

# Include files
USER_CFLAGS += -I include
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

ifdef DEBUG
USER_LDFLAGS = --entry=main
endif

.PHONY: user-test
user-test: user-bin $(USER_OBJECTS) $(USER_BINARIES)	
	
.PHONY: user-bin
user-bin: 
	mkdir -p user/bin	

# Recipe for object files
user/%.o: user/%.c
	$(CC) $(CFLAGS) $(USER_CFLAGS) -c $< -o $@
# Recipe for binary files
user/bin/%: user/%.o libs
	$(LD) $(LDFLAGS) $(USER_LDFLAGS) -o $@ $< $(LIBS) linux/li_proxy.o
