d := $(dir)

ALL_DIRS += $(ARCH_BUILD_DIR)$(d)

ARCH_INCLUDE := $(d)/include
KERNEL_INCLUDE += $(ARCH_INCLUDE)

#TODO
ARCH_SRCS += $(wildcard $(d)*.c)

#KERNEL_SRCS += $(filter-out %main-arch.c, $(ARCH_SRCS))

arch: $(ARCH_OBJS)

$(ARCH_OBJ_DIR)%.o:%.c
	@echo $@
	$(CROSS_CC) $< -MM -MT $(patsubst %.d,%.o,$@) $(KERNEL_INCLUDE) >$@
	$(CROSS_CC) $< -c -o $@ $(KERNEL_INCLUDE)

export RELOCATE_FLAGS := -m elf_i386 -r

i386_DRIVER_OBJS := ata \
	cmos \
	console \
	keyboard \
	pic \
	pit \
	rtc \
	ktime \
	serial \
	fpu

i386_LOCK_OBJS := stdlock
i386_PROC_OBJS := elf iosched proc
i386_SRC_OBJS := devman panic main fsman cpu
i386_SYSCALL_OBJS := sysfile sysproc
i386_TRAP_OBJS := asm trap idt
i386_VM_OBJS := asm pgdir vm_alloc vm
i386_SIGNAL_OBJS := signal
i386_x86_OBJS := asm
i386_CONTEXT_OBJS := context

i386_DRIVER_OBJS := $(addprefix drivers/, $(i386_DRIVER_OBJS))
i386_LOCK_OBJS := $(addprefix lock/, $(i386_LOCK_OBJS))
i386_PROC_OBJS := $(addprefix proc/, $(i386_PROC_OBJS))
i386_SRC_OBJS := $(addprefix src/, $(i386_SRC_OBJS))
i386_SYSCALL_OBJS := $(addprefix syscall/, $(i386_SYSCALL_OBJS))
i386_TRAP_OBJS := $(addprefix trap/, $(i386_TRAP_OBJS))
i386_VM_OBJS := $(addprefix vm/, $(i386_VM_OBJS))
i386_SIGNAL_OBJS := $(addprefix signal/, $(i386_SIGNAL_OBJS))
i386_x86_OBJS := $(addprefix x86/, $(i386_x86_OBJS))
i386_CONTEXT_OBJS := $(addprefix context/, $(i386_CONTEXT_OBJS))

i386_OBJS := \
	$(i386_DRIVER_OBJS) \
	$(i386_LOCK_OBJS) \
	$(i386_PROC_OBJS) \
	$(i386_SRC_OBJS) \
	$(i386_SYSCALL_OBJS) \
	$(i386_TRAP_OBJS) \
	$(i386_VM_OBJS) \
	$(i386_SIGNAL_OBJS) \
	$(i386_x86_OBJS) \
	$(i386_CONTEXT_OBJS)
i386_OBJS := $(addsuffix .o, $(i386_OBJS))
i386_OBJS := $(addprefix $(d), $(i386_OBJS))

ARCH_OBJS += $(patsubst %.S,$(ARCH_BUILD_DIR)%.o,$(i386_OBJS))
ARCH_OBJS += $(patsubst %.c,$(ARCH_BUILD_DIR)%.o,$(ARCH_SRCS))
ARCH_OBJS += $(patsubst %.S,$(ARCH_BUILD_DIR)%.o,$(ARCH_SRCS))

$(ARCH_BUILD_DIR)arch.o:  $(ARCH_OBJS) arch-tools
	$(CROSS_LD) $(RELOCATE_FLAGS) $(ARCH_OBJS) -o arch.o

PHONY += arch-clean
arch-clean:
	cd boot ; \
	make boot-clean
	cd tools ; \
	make tools-clean
	rm -f $(ARCH_OBJS) arch.o trap/idt.S

#TODO: Probably can keep this as a recursive make.
PHONY += arch-tools
arch-tools:
	cd tools ; \
	make tools

trap/idt.S: arch-tools
	./tools/bin/mkvect > ./trap/idt.S

# More rules for ARCH specific targets.
$(ARCH_BUILD_DIR)%.o: %.c
	$(CROSS_CC) $< -MM -MT $(patsubst %.d,%.o,$@) $(KERNEL_INCLUDE) >$@
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(i386_INCLUDE) -c -o $@ $<

$(ARCH_BUILD_DIR)%.o: %.S
	$(CROSS_AS) $(CFLAGS) $(KINCLUDE) -c -o $@ $<
	$(CROSS_AS) $(AFLAGS) $(BUILD_AFLAGS) $(i386_INCLUDE) -c -o $@ $<

-include $(ARCH_SRCS:%.c=$(OBJDIR)%.d)
