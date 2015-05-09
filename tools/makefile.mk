TOOLS := boot-sign \
	mkfs

TOOLS_BINARIES := $(addprefix tools/bin/, $(TOOLS))

tools: tools-dir $(TOOLS_BINARIES)
	
tools-dir:
	mkdir -p tools/bin/

tools/bin/%: tools/%.c
	$(CC) $(CFLAGS) -o $@ $<

tools-clean:
	rm -rf tools/bin/
