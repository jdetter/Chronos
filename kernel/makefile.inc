d := $(dir)

KERNEL_SRCS := $(wildcard $(d)*.c)
KERNEL_INCLUDE := -I$(d)include

ARCH_PATH ?= $(d)arch/$(ARCH)
ARCH_BUILD_DIR ?= $(BUILD_DIR)$(ARCH)-build/

KERNEL_SUBSYSTEMS := $(filter-out %include %arch, $(shell find $(d) -mindepth 1 -maxdepth 1 -type d))
KERNEL_SUBSYSTEMS += $(ARCH_PATH)

define INCLUDE_template
 dir= $(shell dirname $(1))/
 include $(1)
endef

SUBSYSTEMS:=$(addsuffix /makefile.inc,$(KERNEL_SUBSYSTEMS))

$(foreach subsystem,$(SUBSYSTEMS),$(eval $(call INCLUDE_template, $(subsystem))))

KERNEL_OBJS := $(patsubst %.c,$(BUILD_DIR)%.o,$(KERNEL_SRCS))

CLEAN += $(KERNEL_OBJS) $(BUILD_DIR)chronos.o

PHONY += kernel
kernel: $(BUILD_DIR)chronos.o arch

PHONY += kernel-symbols
kernel-symbols: chronos.o 
	$(CROSS_OBJCOPY) --only-keep-debug chronos.o chronos.sym

PHONY += kernel-clean
kernel-clean: arch-clean
	rm -f $(KERNEL_CLEAN)

$(BUILD_DIR)chronos.o: $(KERNEL_OBJS) $(ARCH_BUILD_DIR)arch.o
	$(CROSS_LD) $(LDFLAGS) $(KERNEL_LDFLAGS) -o chronos.o $^

$(BUILD_DIR)%.o: %.S %.d
	$(CROSS_AS) $(DEPS_FLAGS) $(AFLAGS) $(KERNEL_INCLUDE) -c -o $@ $<

$(BUILD_DIR)%.o: %.c %.d
	$(CROSS_CC) $(DEPS_FLAGS) $(CFLAGS) $(KERNEL_INCLUDE) -c -o $@ $<

%.d:;

-include $(KERNEL_SRCS:%.c=$(BUILD_DIR)%.d)
