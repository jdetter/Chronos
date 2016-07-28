TOOLS := \
	cdisk \
	ptabledump \
	boot-imager
TOOLS_BINARIES := $(addprefix bin/, $(TOOLS))

KERNEL_DIR ?= ../kernel
ARCH_DIR ?= $(KERNEL_DIR)/arch/i386
INCLUDE_PATHS := $(KERNEL_DIR)/include include $(ARCH_DIR)/include

INCLUDE := $(addprefix -I,$(INCLUDE_PATHS))

TOOLS_CFLAGS := -D__LINUX__ -DARCH_$(BUILD_ARCH) $(INCLUDE)
TOOLS_CLEAN := bin/ $(TOOLS_BINARIES)

.PHONY: tools
tools: bin $(TOOLS_BINARIES)

clean:
	rm -rf $(TOOLS_CLEAN)

bin:
	mkdir -p bin

bin/%: src/%.c
	$(CC) $(TOOLS_CFLAGS) -o $@ $<
