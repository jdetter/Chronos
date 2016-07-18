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

arch.o: arch-tools $(i386_OBJS)
	$(CROSS_LD) $(RELOCATE_FLAGS) $(i386_OBJS) -o arch.o

.PHONY: boot-stage1.img
boot-stage1.img:
	cd boot ; \
	make boot-stage1.img

.PHONY: boot-stage2.img
boot-stage2.img:
	cd boot ; \
	make boot-stage2.img

.PHONY: arch-clean
arch-clean:
	cd boot ; \
	make boot-clean
	cd tools ; \
	make tools-clean
	rm -f $(i386_OBJS) arch.o trap/idt.S

.PHONY: arch-tools
arch-tools:
	cd tools ; \
	make tools

trap/idt.S: arch-tools
	./tools/bin/mkvect > ./trap/idt.S

%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(i386_INCLUDE) -c -o $@ $<
%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(BUILD_AFLAGS) $(i386_INCLUDE) -c -o $@ $<
