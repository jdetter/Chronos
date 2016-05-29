TOOLS := \
	disk-part
TOOLS_BINARIES := $(addprefix bin/, $(TOOLS))

TOOLS_CFLAGS := -D__LINUX__
TOOLS_CLEAN := bin/ $(TOOLS_BINARIES)

.PHONY: tools
tools: bin $(TOOLS_BINARIES)

tools-clean:
	rm -rf $(TOOLS_CLEAN)

bin:
	mkdir -p bin

bin/%: src/%.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -o $@ $<
