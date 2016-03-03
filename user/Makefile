# Modify the arguments coming in
export CROSS_CC := ../$(CROSS_CC)
export CROSS_LD := ../$(CROSS_LD)
export CROSS_AS := ../$(CROSS_AS)
export CROSS_OBJCOPY := ../$(CROSS_OBJCOPY)

# Other stuff our sub-makes need
export CROSS_CC := $(CROSS_CC)
export CROSS_LD := $(CROSS_LD)
export CROSS_AS := $(CROSS_AS)
export CROSS_OBJCOPY := $(CROSS_OBJCOPY)
export CC := $(CC)
export LD := $(LD)
export AS := $(AS)
export OBJCOPY := $(OBJCOPY)
export LDFLAGS := $(LDFLAGS)
export CFLAGS := $(CFLAGS)
export AFLAGS := $(AFLAGS)

# Specify build targets. Exclude the file extension (e.g. .c or .s)
USER_TARGETS := \
	float-test \
	thread-test \
	exercise \
	shared \
	select-test \
	nc \
	kill \
	tty-test \
	dd \
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

LIBS := -lncurses

# Binary files
USER_BINARIES := $(addprefix bin/, $(USER_TARGETS))
# Create targets
USER_OBJECTS := $(addsuffix .o, $(USER_BINARIES))
# Symbol Files
USER_SYMBOLS := $(addsuffix .sym, $(USER_BINARIES))

# Specify clean targets
USER_CLEAN := \
	bin \
	$(USER_OBJECTS) \
	$(USER_SYMBOLS)

UINCLUDE := \
	-Iinclude

.PHONY: all user-clean
all: bin $(USER_BINARIES)
clean:
	rm -f $(USER_OBJECTS) $(USER_SYMBOLS)
	rm -rf bin

bin: 
	mkdir -p bin

user-symbols: $(USER_SYMBOLS)

# Recipe for binary files
bin/%: src/%.c
	$(CROSS_CC) $(CFLAGS) $(UINCLUDE) -o $@ $< -I user/include -I user/bin $(LIBS)

# Recipe for symbole files
bin/%.sym: bin/%
	$(CROSS_OBJCOPY) --only-keep-debug $< $@
