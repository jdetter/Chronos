TOOLS := \
        mkvect \
	boot-sign \
	boot2-verify

TOOLS_BINARIES := $(addprefix bin/, $(TOOLS))

TOOLS_CFLAGS := \
	-D__LINUX__

TOOLS_CLEAN := bin/ $(TOOLS_BINARIES)

.PHONY: tools tools-clean
tools: bin $(TOOLS_BINARIES)

tools-clean:
	rm -rf $(TOOLS_CLEAN)

bin:
	mkdir -p bin

deps/%.o: deps/%.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -c -o $@ $<

bin/%: src/%.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -c -o $@.o $< 
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -m32 -o $@ $@.o $(TOOLS_DEPS)
