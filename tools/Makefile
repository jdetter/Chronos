TOOLS := boot-sign \
	disk-part \
	boot2-verify \
	mkvect 

TOOLS_BINARIES := $(addprefix bin/, $(TOOLS))

TOOLS_CFLAGS := -D__LINUX__ \
		-I ../kernel \
		-I ../kernel/include \
		-I ../kernel/cache \
		-I ../kernel/arch/i386/include

TOOLS_DEPS := \
	deps/file.o \
	deps/stdlock.o \
	deps/drivers/diskio.o

TOOLS_CLEAN := bin/ $(TOOLS_BINARIES) $(TOOLS_DEPS)

.PHONY: tools
tools: bin $(TOOLS_DEPS) $(TOOLS_BINARIES)

tools-clean:
	rm -rf $(TOOLS_CLEAN)

bin:
	mkdir -p bin

deps/%.o: deps/%.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -c -o $@ $<

bin/%: src/%.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -c -o $@.o $< 
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -o $@ $@.o $(TOOLS_DEPS)
