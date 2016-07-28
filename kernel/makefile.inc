d := $(dir)

ARCH_PATH ?= $(d)arch/$(ARCH)
ARCH_BUILD_DIR ?= $(BUILD_DIR)$(ARCH)/
ALL_DIRS += $(ARCH_OBJ_DIR)

KERNEL_SRCS := $(wildcard $(d)*.c)
KERNEL_INCLUDE := -I$(d)include

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

$(BUILD_DIR)%.o: %.S
	$(CROSS_AS) $< -MM -MT $(patsubst %.d,%.o,$@) $(KERNEL_INCLUDE) >$@
	$(CROSS_AS) $(AFLAGS) $(KINCLUDE) -c -o $@ $<

$(BUILD_DIR)%.o: %.c
	$(CROSS_CC) $< -MM -MT $(patsubst %.d,%.o,$@) $(KERNEL_INCLUDE) >$@
	$(CROSS_CC) $(CFLAGS) $(KINCLUDE) -c -o $@ $<

-include $(KERNEL_SRCS:%.c=$(BUILD_DIR)%.d)
