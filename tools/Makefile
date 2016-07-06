TOOLS := \
	cdisk \
	ptabledump \
	boot-imager
TOOLS_BINARIES := $(addprefix bin/, $(TOOLS))

TOOLS_CFLAGS := -D__LINUX__ -DARCH_$(BUILD_ARCH) -Iinclude/
TOOLS_CLEAN := bin/ $(TOOLS_BINARIES)

.PHONY: tools
tools: bin $(TOOLS_BINARIES)

tools-clean:
	rm -rf $(TOOLS_CLEAN)

bin:
	mkdir -p bin

bin/%: src/%.c
	$(CC) $(TOOLS_CFLAGS) -o $@ $<
