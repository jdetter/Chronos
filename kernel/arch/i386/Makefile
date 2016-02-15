CROSS_CC := gcc
CROSS_AS := gcc
CROSS_LD := ld
CFLAGS := -DARCH_i386 -DARCH_STR="i386" -nostdlib -m32
AFLAGS := -DARCH_i386 -DARCH_STR="i386" -nostdlib -m32

DRIVER_CFLAGS :=
DRIVER_AFLAGS :=
RELOCATE_FLAGS := -m elf_i386 -r


# Erase cruft above ^^^

i386_INCLUDE := \
	-I./ \
	-I../../include \
	-I../../drivers \
	-I../../cache \
	-I../..

i386_CFLAGS := $(i386_INCLUDE) $(i386_CFLAGS)
i386_AFLAGS := $(i386_INCLUDE) $(i386_AFLAGS)

i386_CONTEXT_OBJS := asm
i386_DRIVER_OBJS := ata \
	cmos \
	console \
	keyboard \
	pic \
	pit \
	rtc \
	serial \
	vt102/tty_codes \
	vt102/tty_display \
	vt102/tty_io \
	vt102/tty_kbd
i386_PROC_OBJS := elf
i386_TRAP_OBJS := asm trap
i386_VM_OBJS := asm pgdir vm_alloc vm

i386_CONTEXT_OBJS := $(addprefix context/, $(i386_CONTEXT_OBJS))
i386_DRIVER_OBJS := $(addprefix drivers/, $(i386_DRIVER_OBJS))
i386_PROC_OBJS := $(addprefix proc/, $(i386_PROC_OBJS))
i386_TRAP_OBJS := $(addprefix trap/, $(i386_TRAP_OBJS))
i386_VM_OBJS := $(addprefix vm/, $(i386_VM_OBJS))

i386_OBJS := \
	$(i386_CONTEXT_OBJS) \
	$(i386_DRIVER_OBJS) \
	$(i386_PROC_OBJS) \
	$(i386_TRAP_OBJS) \
	$(i386_VM_OBJS)
i386_OBJS := $(addsuffix .o, $(i386_OBJS))

arch.o: $(i386_OBJS)
	$(CROSS_LD) $(RELOCATE_FLAGS) $(i386_OBJS)

.PHONY: arch-clean
arch-clean:
	rm -f $(i386_OBJS) arch.o

%.o: %.c
	$(CROSS_CC) $(CFLAGS) $(BUILD_CFLAGS) $(i386_CFLAGS) -c -o $@ $<
%.o: %.S
	$(CROSS_AS) $(AFLAGS) $(BUILD_AFLAGS) $(i386_AFLAGS) -c -o $@ $<
