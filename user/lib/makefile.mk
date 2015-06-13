# Specify build targets. Exclude the file extension (e.g. .c or .s)
USER_LIB_TARGETS := \
	stdio \
	stdmem-msetup

# User lib binary targets
USER_LIB_TARGETS := $(addprefix user/lib/, $(USER_LIB_TARGETS))
# Object files
USER_LIB_OBJECTS := $(addsuffix .o, $(USER_LIB_TARGETS))
# Source files
USER_LIB_SOURCE := $(addsuffix .c, $(USER_LIB_TARGETS))

# Specify clean targets
USER_LIB_CLEAN := \
	$(USER_LIB_OBJECTS)

# Include files
USER_LIB_CFLAGS += -I include
# Disable Position Independant Code
USER_LIB_CFLAGS += -fno-pic
# Disable built in functions
USER_LIB_CFLAGS += -fno-builtin
# Disable optimizations that deal with aliases.
USER_LIB_CFLAGS += -fno-strict-aliasing
# Disable stack smashing protection
USER_LIB_CFLAGS += -fno-stack-protector

user-lib: $(USER_LIB_OBJECTS)

# Recipe for object files
user/lib/%.o: user/lib/%.c
	$(CC) $(CFLAGS) $(USER_LIB_CFLAGS) -c $< -o $@
