export RELOCATE_FLAGS := -m elf_i386 -r

export i386_INCLUDE_PATHS := \
	$(shell readlink -e ./include)

export i386_INCLUDE := $(addprefix -I,$(i386_INCLUDE_PATHS))

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
i386_SRC_OBJS := devman panic
i386_SYSCALL_OBJS := sysfile sysproc
i386_TRAP_OBJS := asm trap
i386_VM_OBJS := asm pgdir vm_alloc vm vm_boot
i386_SIGNAL_OBJS := signal
i386_x86_OBJS := asm

i386_DRIVER_OBJS := $(addprefix drivers/, $(i386_DRIVER_OBJS))
i386_LOCK_OBJS := $(addprefix lock/, $(i386_LOCK_OBJS))
i386_PROC_OBJS := $(addprefix proc/, $(i386_PROC_OBJS))
i386_SRC_OBJS := $(addprefix src/, $(i386_SRC_OBJS))
i386_SYSCALL_OBJS := $(addprefix syscall/, $(i386_SYSCALL_OBJS))
i386_TRAP_OBJS := $(addprefix trap/, $(i386_TRAP_OBJS))
i386_VM_OBJS := $(addprefix vm/, $(i386_VM_OBJS))
i386_SIGNAL_OBJS := $(addprefix signal/, $(i386_SIGNAL_OBJS))
i386_x86_OBJS := $(addprefix x86/, $(i386_x86_OBJS))

i386_OBJS := \
	$(i386_DRIVER_OBJS) \
	$(i386_LOCK_OBJS) \
	$(i386_PROC_OBJS) \
	$(i386_SRC_OBJS) \
	$(i386_SYSCALL_OBJS) \
	$(i386_TRAP_OBJS) \
	$(i386_VM_OBJS) \
	$(i386_SIGNAL_OBJS) \
	$(i386_x86_OBJS)
i386_OBJS := $(addsuffix .o, $(i386_OBJS))

arch.o: $(i386_OBJS)
	$(CROSS_LD) $(RELOCATE_FLAGS) $(i386_OBJS) -o arch.o

boot-stage1.img:
	cd boot ; \
	make boot-stage1.img

boot-stage2.img:
	cd boot ; \
	make boot-stage2.img

.PHONY: clean
clean: arch-clean

.PHONY: arch-clean
arch-clean:
	cd boot ; \
	make boot-clean
	rm -f $(i386_OBJS) arch.o


%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(i386_INCLUDE) -c -o $@ $<
%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(BUILD_AFLAGS) $(i386_INCLUDE) -c -o $@ $<
