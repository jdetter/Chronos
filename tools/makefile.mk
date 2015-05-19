TOOLS := boot-sign \
	mkfs \
	disk-part

TOOLS_BINARIES := $(addprefix tools/bin/, $(TOOLS))

TOOLS_CLEAN := tools/bin/

tools: tools-dir $(TOOLS_BINARIES)

tools-dir:
	mkdir -p tools/bin/

tools/bin/%: tools/%.c
	$(CC) $(CFLAGS) -o $@ $<
