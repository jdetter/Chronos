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
	main \
	panic \
	kcond \
	cpu \
	pipe \
	fsman \
	devman \
	iosched \
	ktime \
	signal \
	stdlock \
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

# Include files
KERNEL_CFLAGS := -Ikernel/include -Ikernel/drivers -Ikernel -Ikernel/cache
KERNAL_AFLAGS := $(KERNEL_CFLAGS)
KERNEL_LDFLAGS := -nostdlib --entry=main --section-start=.text=0xFF000000

# Specify files to clean
KERNEL_CLEAN := \
        $(KERNEL_BINS) \
        $(KERNEL_DRIVERS) \
        chronos.o \
        idt.S \
        chronos.sym

.PHONY: kernel-symbols
kernel-symbols: chronos.o 
	$(CROSS_OBJCOPY) --only-keep-debug chronos.o chronos.sym

kernel/chronos.o: kernel/idt.S kernel/idt.o $(KERNEL_OBJS)
	$(CROSS_LD) $(LDFLAGS) $(KERNEL_LDFLAGS) -o chronos.o $(KERNEL_OBJS)

kernel/idt.S: tools/bin/mkvect
	tools/bin/mkvect > kernel/idt.S
%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(KERNEL_AFLAGS) -c -o $@ $<
%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(KERNEL_CFLAGS) -c -o $@ $<

