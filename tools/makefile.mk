TOOLS := boot-sign \
	disk-part \
	boot2-verify \
	mkvect 
#	fsck 
#	ext2.fsck 
#	mkfs 

TOOLS_SOURCE := $(addprefix tools/, $(TOOLS))
TOOLS_SOURCE := $(addsuffix .c, $(TOOLS_SOURCE))

TOOLS_BINARIES := $(addprefix tools/bin/, $(TOOLS))
TOOLS_CLEAN := tools/bin/ tools/deps/

TOOLS_CFLAGS := -D__LINUX__ \
		-I kernel \
		-I kernel/include \
		-I kernel/cache

TOOLS_DEPS := tools/deps/file.o \
		tools/deps/stdlock.o \
		tools/deps/diskio.o \
		tools/deps/vsfs.o 
#		tools/deps/ext2.o \
#		tools/deps/diskcache.o \
#		tools/deps/cache.o \

tools/bin:
	mkdir -p tools/bin
tools/deps:
	mkdir -p tools/deps

.PHONY: tools-deps
tools-deps: tools/bin tools/deps
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/cache/cache.c -o tools/deps/cache.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/cache/diskcache.c -o tools/deps/diskcache.o
#	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/drivers/ext2.c -o tools/deps/ext2.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/stdlock.c -o tools/deps/stdlock.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/drivers/vsfs.c -o tools/deps/vsfs.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/file.c -o tools/deps/file.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/drivers/diskio.c -o tools/deps/diskio.o

tools/bin/%: tools/%.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -c -o $@.o $< 
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -o $@ $@.o $(TOOLS_DEPS)

TOOLS_BUILD := tools/bin tools/deps tools-deps $(TOOLS_BINARIES)
.PHONY: tools
tools: $(TOOLS_BUILD)
