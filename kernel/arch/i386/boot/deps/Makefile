BOOT_STAGE1_DEPS := \
	stage1/ata
BOOT_STAGE1_DEPS := $(addsuffix .o, $(BOOT_STAGE1_DEPS))

BOOT_STAGE2_DEPS := \
	arch-vm/asm \
	arch-vm/pgdir \
	arch-vm/vm_alloc \
	arch-drivers/serial \
	arch-drivers/ata \
	arch-drivers/pic \
	kern-drivers/diskio \
	kern-drivers/ext2 \
	cache/cache \
	cache/diskcache \
	cache/cacheman \
	kern/cpu \
	kern/stdlock \
	kern/file \
	kern/stdlib
BOOT_STAGE2_DEPS := $(addsuffix .o, $(BOOT_STAGE2_DEPS))

BOOT_STAGE2_INCLUDE := \
	-Iinclude/
BOOT_STAGE2_CFLAGS := $(BOOT_STAGE2_INCLUDE) $(BOOT_STAGE2_CFLAGS)
BOOT_STAGE2_AFLAGS := $(BOOT_STAGE2_INCLUDE) $(BOOT_STAGE2_AFLAGS)

BOOT_STAGE2_CFLAGS := -I./include $(BOOT_STAGE2_CFLAGS)
BOOT_STAGE2_AFLAGS := -I./include $(BOOT_STAGE2_AFLAGS)

stage1-deps.o: $(BOOT_STAGE1_DEPS)
	$(CROSS_LD) $(RELOCATE_FLAGS) $(BOOT_STAGE1_DEPS) -o stage1-deps.o

stage2-deps.o: $(BOOT_STAGE2_DEPS)
	$(CROSS_LD) $(RELOCATE_FLAGS) $(BOOT_STAGE2_DEPS) -o stage2-deps.o

.PHONY: deps-clean
deps-clean:
	rm -f $(BOOT_STAGE2_DEPS) stage2-deps.o \
		$(BOOT_STAGE1_DEPS) stage1-deps.o

%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(BOOT_STAGE2_CFLAGS) $(i386_INCLUDE) -D__BOOT_STRAP__ -c -o $@ $< 
%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(BUILD_AFLAGS) $(BOOT_STAGE2_AFLAGS) $(i386_INCLUDE) -D__BOOT_STRAP__ -c -o $@ $< 
