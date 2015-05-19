# Specify build targets. Exclude the file extension (e.g. .c or .s)
KERNEL_OBJECTS := \
	main \
	stdarg

# Specify driver files. Exclude all file extensions
KERNEL_DRIVERS := \
	ata \
	keyboard \
	pic\
	pit \
	console

# Add kernel/ before all of the kernel targets
KERNEL_OBJECTS := $(addprefix kernel/, $(KERNEL_OBJECTS))
KERNEL_OBJECTS := $(addsuffix .o, $(KERNEL_OBJECTS))

# put drivers into the form kernel/drivers/file.o
KERNEL_DRIVERS := $(addprefix kernel/drivers/, $(KERNEL_DRIVERS))
KERNEL_DRIVERS := $(addsuffix .o, $(KERNEL_DRIVERS))

# Specify files to clean
KERNEL_CLEAN := \
        $(KERNEL_OBJECTS) \
        $(KERNEL_DRIVERS) \
        kernel/chronos.img \
        kernel/chronos.o \
        kernel/boot/bootasm.o \
        kernel/boot/boot-stage1.img \
        kernel/boot/boot-stage1.o \
        chronos.img \
	boot-stage1.sym \
	chronos.sym

# Include files
KERNEL_CFLAGS += -I include
# Include driver headers
KERNEL_CFLAGS += -I kernel/drivers
# Include libraries
KERNEL_CFLAGS += -I lib

# Don't link the standard library
KERNEL_LDFLAGS := -nostdlib
# Set the entry point
KERNEL_LDFLAGS += --entry=main
# Set the memory location where the kernel will be placed
KERNEL_LDFLAGS += --section-start=.text=0x100000
# KERNEL_LDFLAGS += --omagic

kernel-symbols: kernel/chronos.o
	$(OBJCOPY) --only-keep-debug kernel/chronos.o chronos.sym
	$(OBJCOPY) --only-keep-debug kernel/boot/boot-stage1.o boot-stage1.sym

chronos.img: kernel/boot/boot-stage1.img kernel/chronos.o
	dd if=/dev/zero of=chronos.img bs=512 count=2048
	dd if=kernel/boot/boot-stage1.img of=chronos.img count=1 bs=512 conv=notrunc seek=0


kernel/chronos.o: includes $(KERNEL_OBJECTS) $(KERNEL_DRIVERS)
	$(LD) $(LDFLAGS) $(KERNEL_LDFLAGS) -o kernel/chronos.o $(KERNEL_OBJECTS) $(KERNEL_DRIVERS) $(INCLUDES)

kernel/boot/boot-stage1.img:
	$(AS) $(ASFLAGS) $(BUILD_ASFLAGS) -c -o kernel/boot/bootasm.o kernel/boot/bootasm.S
	$(LD) $(LDFLAGS) --section-start=.text=0x7c00 --entry=start  -o kernel/boot/boot-stage1.o kernel/boot/bootasm.o
	$(OBJCOPY) -S -O binary -j .text kernel/boot/boot-stage1.o kernel/boot/boot-stage1.img
	

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) $(KERNEL_CFLAGS) $(BUILD_CFLAGS) -c -o $@ $<


kernel/drivers/%.o: kernel/drivers/%.c
	$(CC) $(CFLAGS) $(KERNEL_CFLAGS) $(BUILD_CFLAGS)-c -o $@ $<
