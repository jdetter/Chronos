
# Export the include folders for the kernel.
export KINCLUDE_PATHS := \
	$(shell readlink -e include)

export KINCLUDE := $(addprefix -I, $(KINCLUDE_PATHS))

# All files that need compiling (excluding drivers)
KERNEL_OBJS := \
	syscall/syscall \
	syscall/sysnet \
	syscall/syssig \
	syscall/sysproc \
	syscall/sysfile \
	syscall/sysutil \
	syscall/sysmmap \
	cache/storagecache \
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
	pipe \
	fsman \
	file \
	stdlib \
	ioctl \
	main \
	stdlock \
	reboot \
	devman

# Generic drivers 
KERNEL_DRIVERS := \
	ext2 \
	storageio \
	lwfs \
	raid \
	tty \
	vt102/tty_display \
	vt102/tty_codes \
	vt102/tty_io \
	vt102/tty_kbd

KERNEL_DRIVERS := $(addprefix drivers/, $(KERNEL_DRIVERS))

KERNEL_OBJS := \
	$(KERNEL_OBJS) \
	$(KERNEL_DRIVERS)

KERNEL_OBJS := $(addsuffix .o, $(KERNEL_OBJS))

# Include files
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

.PHONY: boot-stage1.img
boot-stage1.img:
	cd arch/$(BUILD_ARCH) ; \
		make boot-stage1.img

.PHONY: boot-stage2.img
boot-stage2.img:
	cd arch/$(BUILD_ARCH) ; \
		make boot-stage2.img

chronos.o: $(KERNEL_OBJS)
	cd arch/$(BUILD_ARCH) ; \
		make arch.o
	$(CROSS_LD) $(LDFLAGS) $(KERNEL_LDFLAGS) -o chronos.o $(KERNEL_OBJS) arch/$(BUILD_ARCH)/arch.o

%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(KINCLUDE) -c -o $@ $<

%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(KINCLUDE) -c -o $@ $<

