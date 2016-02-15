TOOLS := boot-sign \
	disk-part \
	boot2-verify \
	mkvect 
#	ext2.fsck 
#	mkfs 
#	lwfs.fsck \

TOOLS_BINARIES := $(addprefix bin/, $(TOOLS))
TOOLS_CLEAN := bin/

TOOLS_CFLAGS := -D__LINUX__ \
		-I kernel \
		-I kernel/include \
		-I kernel/cache

TOOLS_DEPS := \
	deps/file.o \
	deps/stdlock.o \
	deps/drivers/diskio.o

.PHONY: tools
tools: tools/bin $(TOOLS_DEPS)

tools/bin:
	mkdir -p tools/bin

.PHONY: tools-deps
tools-deps: tools/bin
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/cache/cache.c -o tools/deps/cache.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/cache/diskcache.c -o tools/deps/diskcache.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/stdlock.c -o tools/deps/stdlock.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/drivers/vsfs.c -o tools/deps/vsfs.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/file.c -o tools/deps/file.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/drivers/diskio.c -o tools/deps/diskio.o
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c -m32 kernel/drivers/lwfs.c -o tools/deps/lwfs.o

deps/%.o: deps/%.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -o $@ $<

tools/bin/%: tools/%.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -c -o $@.o $< 
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -o $@ $@.o $(TOOLS_DEPS)

TOOLS_BUILD := tools/bin tools/deps tools-deps $(TOOLS_BINARIES)
.PHONY: tools
tools: $(TOOLS_BUILD)
