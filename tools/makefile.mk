TOOLS := boot-sign \
	mkfs \
	disk-part \
	fsck \
	boot2-verify \
	mkvect \
	ext2.fsck

TOOLS_SOURCE := $(addprefix tools/, $(TOOLS))
TOOLS_SOURCE := $(addsuffix .c, $(TOOLS_SOURCE))

TOOLS_BINARIES := $(addprefix tools/bin/, $(TOOLS))
TOOLS_CLEAN := tools/bin/ tools/deps/

TOOLS_CFLAGS := -D__LINUX__ \
		-I tools/include

TOOLS_DEPS := tools/deps/vsfs.o \
		tools/deps/file.o \
		tools/deps/stdlock.o \
		tools/deps/ext2.o

tools/bin:
	mkdir -p tools/bin
tools/deps:
	mkdir -p tools/deps

.PHONY: tools-deps
tools-deps: tools/bin tools/deps
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/stdlock.c -o tools/deps/stdlock.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/drivers/vsfs.c -o tools/deps/vsfs.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/file.c -o tools/deps/file.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/drivers/ext2.c -o tools/deps/ext2.o

tools/bin/%: tools/%.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -c -o $@.o $< 
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -o $@ $@.o $(TOOLS_DEPS)


TOOLS_BUILD := tools/bin tools/deps tools-deps $(TOOLS_BINARIES)
.PHONY: tools
tools: $(TOOLS_BUILD)
