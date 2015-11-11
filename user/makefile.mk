# Specify build targets. Exclude the file extension (e.g. .c or .s)
USER_TARGETS := \
	sync \
	signal-test \
	compile-test \
	fs \
	kill-test \
	malloc-test \
	getlogin-test \
	sleep-test \
	ttyname-test \
	types \
	init \
	example \
	otherexample \
	cat \
	echo \
	login \
	mv \
	rmdir \
	test \
	stress \
	touch \
	pwd \
	stall \
	crash \
	chmod \
	chown \
	date \
	stack-test \
	exec-test \
	env \
	ret-test \
	sh \
	ls \
	rm \
	mkdir \
	opendir-test

#	syscall-test \

# stat

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

# Disable Position Independant Code
USER_CFLAGS += -fno-pic
# Disable built in functions
USER_CFLAGS += -fno-builtin
# Disable optimizations that deal with aliases.
USER_CFLAGS += -fno-strict-aliasing
# Disable stack smashing protection
USER_CFLAGS += -fno-stack-protector

USER_BUILD := kernel/file.o kernel/stdlock.o user/syscall.o user/bin $(USER_BINARIES) 
	
user/bin: 
	mkdir -p user/bin
	cp kernel/include/file.h user/bin/
	cp kernel/include/stdlock.h user/bin/

user-symbols: $(USER_SYMBOLS)

user/syscall.o:
	$(CROSS_AS) $(ASFLAGS) $(BUILD_ASFLAGS) -I kernel/include -c user/syscall.S -o user/syscall.o

# Recipe for binary files
user/bin/%: user/%.c
	$(CROSS_CC) $(CFLAGS) -o $@ $< -I user/include -I user/bin

# Recipe for symbole files
user/bin/%.sym: user/bin/%
	$(CROSS_OBJCOPY) --only-keep-debug $< $@
