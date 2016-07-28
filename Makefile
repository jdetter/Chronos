# Use the new tool chain to build executables.
export TARGET=i686-pc-chronos-
export ARCH=i386
export BUILD_DIR:=build/
export TARGET_TRIPLE=i686-pc-chronos-
export BUILD_ARCH := i386
TOOL_DIR=../tools/bin


CROSS_CC := $(TOOL_DIR)/$(TARGET)gcc
CROSS_LD := $(TOOL_DIR)/$(TARGET)ld
CROSS_AS := $(TOOL_DIR)/$(TARGET)gcc
CROSS_OBJCOPY := $(TOOL_DIR)/$(TARGET)objcopy
export CROSS_CC := $(shell readlink -e "$(CROSS_CC)")
export CROSS_LD := $(shell readlink -e "$(CROSS_LD)")
export CROSS_AS := $(shell readlink -e "$(CROSS_AS)")
export CROSS_OBJCOPY := $(shell readlink -e "$(CROSS_OBJCOPY)")

CROSS_CC := ./test.sh
CROSS_LD := ./test.sh
CROSS_AS := ./test.sh
CROSS_OBJCOPY := ./test.sh

export HOST_CC	   :=gcc
export HOST_LD	   :=ld
export HOST_AS	   :=gcc
export HOST_OBJCOPY:=objcopy


TARGET_SYSROOT := ../sysroot
export TARGET_SYSROOT := $(shell readlink -e $(TARGET_SYSROOT))
export USER := $(shell whoami)
export BOOT2_IMAGER := $(shell pwd)/tools/bin/boot-imager


# use host to configure the tools

export LDFLAGS := 
export CFLAGS := -ggdb -Werror -Wall -gdwarf-2 -fno-common -DARCH_$(BUILD_ARCH) -DARCH_STR=$(BUILD_ARCH) -fno-builtin -fno-stack-protector $(CFLAGS)
export AFLAGS := -ggdb -Werror -Wall -DARCH_$(BUILD_ARCH) -DARCH_STR=$(BUILD_ARCH) $(AFLAGS)

# If the target isn't a clean, make the build dir.
ifneq ($(filter %clean ,$(MAKECMDGOALS)),)
NOMAKEDIR:=1
endif

PHONY := all build-dirs clean dist-clean
all: kernel

dir:=kernel/
include kernel/makefile.inc

ALL_SRCS += $(KERNEL_SRCS)
ALL_SRC_DIRS += $(dir $(ALL_SRCS))
ALL_DIRS += $(addprefix $(BUILD_DIR),$(ALL_SRC_DIRS))

clean:
	rm -rf $(CLEAN)

dist-clean:
	$(MAKE) clean
	rm -rf $(OBJDIR)

ifndef NOMAKEDIR
$(shell mkdir -p $(OBJDIR) $(ALL_DIRS))
endif

#TODO: FIX below here.
#TODO: Fix PHONY
#######################

QEMU := qemu-system-$(BUILD_ARCH)

# Uncomment this lines to turn off all output
# export CFLAGS := -DRELEASE $(CFLAGS)
# export AFLAGS := -DRELEASE $(AFLAGS)

# Uncomment these lines to turn on all output (log flood)
# export CFLAGS := -DDEBUG $(CFLAGS)
# export AFLAGS := -DDEBUG $(AFLAGS)

# Create a 128MB Hard drive
FS_TYPE := ext2.img
FS_DD_BS := 512
FS_DD_COUNT := 2097152
FS_START := 2048
DISK_SZ := 1074790400

# Size of the second boot sector 
BOOT_STAGE2_SECTORS := 140

.PHONY: all
all: chronos.img

.PHONY: chronos.img
chronos.img: 
	cd tools/ ; \
	make tools || exit 1
	cd kernel/ ; \
	make chronos.o || exit 1 ; \
	make boot-stage1.img || exit 1 ; \
	make boot-stage2.img || exit 1 ;
	cd user ; \
	make
	make $(FS_TYPE)
	echo "yes" | ./tools/bin/cdisk --part1=$(FS_START),$(FS_DD_COUNT),$(FS_TYPE),EXT2 -s $(DISK_SZ) -l 512 -b kernel/arch/$(BUILD_ARCH)/boot/boot-stage1.img --stage-2=kernel/arch/$(BUILD_ARCH)/boot/boot-stage2.img,1,$(BOOT_STAGE2_SECTORS) ./chronos.img

chronos-multiboot.img:
	cd tools ; \
	make tools || exit 1
	cd kernel/ ; \
	make chronos.o || exit 1 ; \
	make multiboot.o || exit 1
	cd user ; \
	make
	make $(FS_TYPE)

virtualbox: tools chronos.img
	./tools/virtualbox.sh

vsfs.img: $(TOOLS_BUILD) kernel/chronos.o $(USER_BUILD)
	mkdir -p fs
	mkdir -p fs/boot
	mkdir -p fs/bin
	cp kernel/chronos.o fs/boot/chronos.elf
	cp -R user/bin/* fs/bin/
	cp -R ../sysroot/* fs/
	./tools/bin/mkfs -i 8192 -s 134217728 -r fs $(FS_TYPE)
#	./tools/bin/mkfs -i 128 -s 16777216 -r fs fs.img

.PHONY: ext2.img
ext2.img: kernel/chronos.o $(USER_BUILD)
	echo "Super user privileges are needed for loop mounting..."
	sudo echo ""
	dd if=/dev/zero of=./ext2.img bs=$(FS_DD_BS) count=$(FS_DD_COUNT) seek=0
	echo "yes" | mkfs.ext2 ./ext2.img
	rm -rf tmp
	mkdir -p tmp
	sudo mount -o loop ./ext2.img ./tmp
	sudo chown -R $(USER):$(USER) tmp
	cp -R $(TARGET_SYSROOT)/* ./tmp/
	bash ./tools/src/gensysskel.sh
	cp -R ./sysskel/. ./tmp
	mkdir -p ./tmp/bin
	cp -R ./user/bin/* ./tmp/bin/
	mkdir -p ./tmp/boot
	cp ./kernel/chronos.o ./tmp/boot/chronos.elf
	cd tmp
	sync
	cd ..
	sudo umount -l ./tmp
	rm -rf tmp

ext2-grub.img:
	echo "Super user privileges are needed for loop mounting..."
	sudo echo ""
	dd if=/dev/zero of=./ext2.img bs=$(FS_DD_BS) count=$(FS_DD_COUNT) seek=0
# Create the partition table
	printf "n\n\n\n\n\nw\n" | fdisk -b 512 ext2.img
# Create the file system
	echo "yes" | mkfs.ext2 -E offset=1048576 ./ext2.img
	rm -rf tmp
	mkdir -p tmp
	sudo mount -o loop,offset=1048576 ./ext2.img ./tmp
	sudo chown -R $(USER):$(USER) tmp
	cp -R $(TARGET_SYSROOT)/* ./tmp
	sudo umount ./tmp
	rm -rf tmp
	sudo grub-install ./ext2.img

fsck: fs.img tools/bin/fsck
	tools/bin/fsck fs.img

QEMU_CPU_COUNT := -smp 1
QEMU_BOOT_DISK := chronos.img
QEMU_MAX_RAM := -m 512M
# QEMU_NOX := -nographic

QEMU_OPTIONS := $(QEMU_CPU_COUNT) $(QEMU_MAX_RAM) $(QEMU_NOX) $(QEMU_BOOT_DISK)

.PHONY: qemu qemu-gdb qemu-x qemu-x-gdb

# Launch current build recipes
run:
	$(QEMU) -nographic $(QEMU_OPTIONS)

run-x:
	$(QEMU) $(QEMU_OPTIONS)

run-gdb: user/bin $(USER_BUILD)
	cd kernel ; \
	make kernel-symbols ; \
	mv *.sym ..
	cd user ; \
	make user-symbols ; \
	mv bin/*.sym ..
	$(QEMU) -nographic $(QEMU_OPTIONS) -s -S

run-x-gdb: kernel-symbols user/bin $(USER_BUILD) user-symbols
	$(QEMU) $(QEMU_OPTIONS) -s -S

# Export entire file system
export-fs:
	rm -f ext2.img
	dd if=./chronos.img of=./ext2.img ibs=512 skip=$(FS_START)
export-logs: export-fs
	rm -rf logs
	mkdir -p chronos-fs
	sudo mount -o loop ./ext2.img ./chronos-fs
	sudo cp -R ./chronos-fs/var/log ./logs
	sudo umount ./chronos-fs
	rmdir chronos-fs
	sudo chown -R $(USER):$(USER) ./logs
	sudo chmod 700 ./logs

PHONY +=export-logs export-fs run-x-gdb run-gdb run-x run

soft-clean:
	rm -rf $(USER_CLEAN) $(KERNEL_CLEAN) $(TOOLS_CLEAN)

# Deploy recipes
deploy-base: clean
	rm -f chronos-deploy.tar
	tar cf chronos-deploy.tar *
	scp -P 8081 ./chronos-deploy.tar ubuntu@localhost:/home/ubuntu/Chronos
	rm -f chronos-deploy.tar
	ssh ubuntu@localhost -p 8081 "/bin/build-gdb.sh"
	scp -P 8081 ubuntu@localhost:/home/ubuntu/Chronos/chronos/chronos.img ./chronos.img
deploy-base-gdb: deploy-base
	scp -P 8081 ubuntu@localhost:/home/ubuntu/Chronos/chronos/symbols.tar ./symbols.tar
	tar xf symbols.tar
	rm -f symbols.tar
deploy: deploy-base run
deploy-x: deploy-base run-x
deploy-gdb: deploy-base-gdb
	$(QEMU) -nographic $(QEMU_OPTIONS) -s -S
deploy-x-gdb: deploy-base-gdb
	$(QEMU) $(QEMU_OPTIONS) -s -S

patch: soft-clean kernel/chronos.o kernel/boot/boot-stage1.img  kernel/boot/boot-stage2.img $(USER_BUILD)
	tools/bin/boot-sign kernel/boot/boot-stage1.img
	dd if=kernel/boot/boot-stage1.img of=chronos.img count=1 bs=512 conv=notrunc seek=0
	dd if=kernel/boot/boot-stage2.img of=chronos.img count=62 bs=512 conv=notrunc seek=1
	./tools/bin/fsck -s 64 chronos.img cp kernel/chronos.o /boot/chronos.elf

PHONY+=clean patch deploy-x-gdb deploy-x deploy dploy-base-gdb deploy-base soft-clean
#clean: 
#	cd tools ; \
#	make tools-clean
#	cd kernel ; \
#	make kernel-clean
#	cd user; \
#	make clean
#	rm -rf $(KERNEL_CLEAN) $(TOOLS_CLEAN) $(LIBS_CLEAN) $(USER_CLEAN) fs fs.img chronos.img $(USER_LIB_CLEAN) .bochsrc bochsout.txt chronos.vdi tmp ext2.img *.sym

.PHONY: $(PHONY)
