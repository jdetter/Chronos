# Stage 1 configuration
BOOT_STAGE1_LDFLAGS := --section-start=.text=0x7c00 --entry=start

# Stage 2 configuration
BOOT_STAGE2_LDFLAGS := \
		--entry=main \
		--section-start=.text=0x7E00 \
                --section-start=.data=0x17E00 \
                --section-start=.rodata=0x18600 \
                --section-start=.bss=0x18e00
# These should reflect the above sector counts
BOOT_STAGE2_TEXT_START := 0
BOOT_STAGE2_TEXT_SZ := 128
# How many sectors do our data sections get?
BOOT_STAGE2_SECT_COUNT := 4
# Where do each of the simple sections start on disk?
BOOT_STAGE2_DATA_START := 128
BOOT_STAGE2_RODATA_START := 132
BOOT_STAGE2_BSS_START := 136
# How many sectors does stage 2 take up?
BOOT_STAGE2_SECTORS := 140

BOOT_OBJS := \
	stage1 \
	stage2 \
	stage2_jmp
BOOT_OBJS := $(addsuffix .o, $(BOOT_OBJS))

.PHONY: boot-clean
boot-clean:
	cd deps ; \
	make deps-clean 
	rm -f  \
		boot-stage1.o boot-stage1.img \
		boot-stage2.o boot-stage2.img 

boot-stage1.img: $(BOOT_OBJS)
	cd deps; \
	make stage1-deps.o

	$(CROSS_LD) $(LDFLAGS) $(BOOT_STAGE1_LDFLAGS) -o ./boot-stage1.o stage1.o ./deps/stage1-deps.o
	$(CROSS_OBJCOPY) -S -O binary -j .text boot-stage1.o boot-stage1.img
	# Sign the boot sector
	../../../../tools/bin/boot-sign ./boot-stage1.img

boot-stage2.img: $(BOOT_OBJS)
	cd deps ; \
	make stage2-deps.o

	# Link the second boot stage 
	$(CROSS_LD) $(LDFLAGS) $(BOOT_STAGE2_LDFLAGS) \
		-o boot-stage2.o stage2.o stage2_jmp.o ./deps/stage2-deps.o

	# Extract the program parts
	$(CROSS_OBJCOPY) -O binary -j .text boot-stage2.o boot-stage2.text
	$(CROSS_OBJCOPY) -O binary -j .data boot-stage2.o boot-stage2.data
	$(CROSS_OBJCOPY) -O binary -j .rodata boot-stage2.o boot-stage2.rodata
	$(CROSS_OBJCOPY) -O binary -j .bss boot-stage2.o boot-stage2.bss
	../../../../tools/bin/boot2-verify
	dd if=boot-stage2.text of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_TEXT_SZ) seek=0
	dd if=boot-stage2.data of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_DATA_START)
	dd if=boot-stage2.rodata of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_RODATA_START)
	dd if=boot-stage2.bss of=boot-stage2.img bs=512 \
		count=$(BOOT_STAGE2_SECT_COUNT) seek=$(BOOT_STAGE2_BSS_START)

%.o: %.c
	echo "$(i386_INCLUDE)"
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(i386_INCLUDE) -D__BOOT_STRAP__ -c -o $@ $< 
%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(BUILD_AFLAGS) $(i386_INCLUDE) -D__BOOT_STRAP__ -c -o $@ $<
