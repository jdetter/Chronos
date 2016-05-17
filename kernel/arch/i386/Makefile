# Modify the arguments coming in
export CROSS_CC := ../../$(CROSS_CC)
export CROSS_LD := ../../$(CROSS_LD)
export CROSS_AS := ../../$(CROSS_AS)
export CROSS_OBJCOPY := ../../$(CROSS_OBJCOPY)
export KINCLUDE_PATHS := $(addprefix ../../,$(KINCLUDE_PATHS))

# Other stuff our sub-makes need
export CC := $(CC)
export LD := $(LD)
export AS := $(AS)
export OBJCOPY := $(OBJCOPY)
export LDFLAGS := $(LDFLAGS)
export CFLAGS := $(CFLAGS)
export AFLAGS := $(AFLAGS)

export RELOCATE_FLAGS := -m elf_i386 -r

export i386_INCLUDE_PATHS := \
	$(KINCLUDE_PATHS) \
	./ \
	include \
	drivers \
	trap

i386_INCLUDE := $(addprefix -I,$(i386_INCLUDE_PATHS))

i386_CONTEXT_OBJS := asm
i386_DRIVER_OBJS := ata \
	cmos \
	console \
	keyboard \
	pic \
	pit \
	rtc \
	ktime \
	serial

i386_LOCK_OBJS := stdlock
i386_PROC_OBJS := elf iosched proc
i386_SRC_OBJS := devman fsman main panic
i386_SYSCALL_OBJS := sysfile sysproc
i386_TRAP_OBJS := asm trap
i386_VM_OBJS := asm pgdir vm_alloc vm
i386_SIGNAL_OBJS := signal

i386_CONTEXT_OBJS := $(addprefix context/, $(i386_CONTEXT_OBJS))
i386_DRIVER_OBJS := $(addprefix drivers/, $(i386_DRIVER_OBJS))
i386_LOCK_OBJS := $(addprefix lock/, $(i386_LOCK_OBJS))
i386_PROC_OBJS := $(addprefix proc/, $(i386_PROC_OBJS))
i386_SRC_OBJS := $(addprefix src/, $(i386_SRC_OBJS))
i386_SYSCALL_OBJS := $(addprefix syscall/, $(i386_SYSCALL_OBJS))
i386_TRAP_OBJS := $(addprefix trap/, $(i386_TRAP_OBJS))
i386_VM_OBJS := $(addprefix vm/, $(i386_VM_OBJS))
i386_SIGNAL_OBJS := $(addprefix signal/, $(i386_SIGNAL_OBJS))

i386_OBJS := \
	$(i386_CONTEXT_OBJS) \
	$(i386_DRIVER_OBJS) \
	$(i386_LOCK_OBJS) \
	$(i386_PROC_OBJS) \
	$(i386_SRC_OBJS) \
	$(i386_SYSCALL_OBJS) \
	$(i386_TRAP_OBJS) \
	$(i386_VM_OBJS) \
	$(i386_SIGNAL_OBJS)
i386_OBJS := $(addsuffix .o, $(i386_OBJS))

arch.o: $(i386_OBJS)
	$(CROSS_LD) $(RELOCATE_FLAGS) $(i386_OBJS) -o arch.o

boot-stage1.img:
	cd boot ; \
	make boot-stage1.img

boot-stage2.img:
	cd boot ; \
	make boot-stage2.img

.PHONY: arch-clean
arch-clean:
	cd boot ; \
	make boot-clean
	rm -f $(i386_OBJS) arch.o

%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(i386_INCLUDE) -c -o $@ $<
%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(BUILD_AFLAGS) $(i386_INCLUDE) -c -o $@ $<
