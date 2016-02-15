# Modify the arguments coming in
export CROSS_CC := "../$(CROSS_CC)"
export CROSS_LD := "../$(CROSS_LD)"
export CROSS_AS := "../$(CROSS_AS)"
export CROSS_OBJCOPY := "../$(CROSS_OBJCOPY)"

# All files that need compiling (excluding drivers)
KERNEL_BINS := \
	syscall/syscall \
	syscall/sysnet \
	syscall/syssig \
	syscall/sysproc \
	syscall/sysfile \
	syscall/sysutil \
	syscall/sysmmap \
	cache/diskcache \
	cache/cacheman \
	cache/cache \
	vm/vm_share \
	vm/vm_cow \
	proc/desc \
	proc/proc \
	proc/sched \
	klog \
	netman \
	panic \
	kcond \
	cpu \
	pipe \
	fsman \
	file \
	stdlib \
	ioctl \
	idt

# Generic drivers 
KERNEL_DRIVERS := \
        ext2 \
	diskio \
	lwfs \
	raid

KERNEL_DRIVERS := $(addprefix drivers/, $(KERNEL_DRIVERS))

KERNEL_OBJS := \
	$(KERNEL_BINS) \
	$(KERNEL_DRIVERS)

KERNEL_OBJS := $(addsuffix .o, $(KERNEL_OBJS))

# Include files
KERNEL_CFLAGS := -Iinclude -Idrivers -Icache -I.
KERNAL_AFLAGS := $(KERNEL_CFLAGS)
KERNEL_LDFLAGS := -nostdlib --entry=main --section-start=.text=0xFF000000

# Specify files to clean
KERNEL_CLEAN := \
        $(KERNEL_OBJS) \
        chronos.o \
        idt.S \
        chronos.sym

.PHONY: kernel-symbols
kernel-symbols: chronos.o 
	$(CROSS_OBJCOPY) --only-keep-debug chronos.o chronos.sym

.PHONY: kernel-clean
kernel-clean:
	cd arch/$(BUILD_ARCH)/ ; \
	make arch-clean
	rm -f $(KERNEL_CLEAN)

chronos.o: idt.S $(KERNEL_OBJS)
	cd arch/$(BUILD_ARCH) ; \
	make arch.o
	$(CROSS_LD) $(LDFLAGS) $(KERNEL_LDFLAGS) -o chronos.o $(KERNEL_OBJS) arch/$(BUILD_ARCH)/arch.o

idt.S: ../tools/bin/mkvect
	../tools/bin/mkvect > ./idt.S
%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(KERNEL_AFLAGS) -c -o $@ $<
%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(KERNEL_CFLAGS) -c -o $@ $<

