# Specify build targets. Exclude the file extension (e.g. .c or .s)
USER_TARGETS:= \ 
	example

# Add user/ before all of the program names
USER_TARGETS := $(addprefix user/, $(USER_TARGETS))

# Disable standard system includes
USER_CFLAGS += -nostdinc

# Disable Position Independant Code
USER_CFLAGS += -fno-pic

# Disable built in functions
USER_CFLAGS += -fno-builtin

# Disable optimizations that deal with aliases.
USER_CFLAGS += -fno-strict-aliasing

# Disable stack smashing protection
USER_CFLAGS += -fno-stack-protector


# Recipe for user objects
user/%.o: user/%.c
	$(CC) $(CFLAGS) $(USER_CFLAGS) -c -o $@ $<

user-progs:
	@echo Making user programs...
	
	
