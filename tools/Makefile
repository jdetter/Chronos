TOOLS := \
	cdisk \
	ptabledump
TOOLS_BINARIES := $(addprefix bin/, $(TOOLS))

ifndef $(CC)
	CC := gcc
	CFLAGS := -ggdb -gdwarf-2
endif

TOOLS_CFLAGS := -D__LINUX__ -Iinclude/
TOOLS_CLEAN := bin/ $(TOOLS_BINARIES)

.PHONY: tools
tools: bin $(TOOLS_BINARIES)

tools-clean:
	rm -rf $(TOOLS_CLEAN)

bin:
	mkdir -p bin

bin/%: src/%.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -o $@ $<
