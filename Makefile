# Use the new tool chain to build executables.
TARGET=i686-pc-chronos-
export BUILD_ARCH := i386
TOOL_DIR=../tools/bin
export CROSS_CC := $(TOOL_DIR)/$(TARGET)gcc
export CROSS_LD := $(TOOL_DIR)/$(TARGET)ld
export CROSS_AS := $(TOOL_DIR)/$(TARGET)gcc
export CROSS_OBJCOPY := $(TOOL_DIR)/$(TARGET)objcopy
export TARGET_SYSROOT := ../sysroot
export USER := $(shell whoami)

# use host to configure the tools
export CC=gcc
export LD=ld
export AS=gcc
export OBJCOPY=objcopy

export LDFLAGS := 
export CFLAGS := -ggdb -Werror -Wall -gdwarf-2 -fno-common -DARCH_$(BUILD_ARCH) -DARCH_STR=$(BUILD_ARCH) -fno-builtin -fno-stack-protector $(CFLAGS)
export AFLAGS := -ggdb -Werror -Wall -DARCH_$(BUILD_ARCH) -DARCH_STR=$(BUILD_ARCH) $(AFLAGS)
QEMU := qemu-system-i386

# Uncomment this lines to turn off all output
export CFLAGS := -DRELEASE $(CFLAGS)
export AFLAGS := -DRELEASE $(AFLAGS)

# Create a 128MB Hard drive
FS_TYPE := ext2.img
FS_DD_BS := 4096
FS_DD_COUNT := 262144
FS_START := 2048

# Size of the second boot sector 
BOOT_STAGE2_SECTORS := 140

.PHONY: all
all: chronos.img

chronos.img: 
	cd tools/ ; \
	make tools
	cd kernel/ ; \
	make chronos.o ; \
	make boot-stage1.img ; \
	make boot-stage2.img 
	cd user ; \
	make
	make $(FS_TYPE)
	dd if=/dev/zero of=chronos.img bs=512 count=2048
	dd if=kernel/arch/$(BUILD_ARCH)/boot/boot-stage1.img of=chronos.img count=1 bs=512 conv=notrunc seek=0
	dd if=kernel/arch/$(BUILD_ARCH)/boot/boot-stage2.img of=chronos.img count=$(BOOT_STAGE2_SECTORS) bs=512 conv=notrunc seek=1
	dd if=$(FS_TYPE) of=chronos.img bs=512 conv=notrunc seek=$(FS_START)

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

kernel/idt.c:
	tools/bin/mkvect > kernel/idt.c

QEMU_CPU_COUNT := -smp 1
QEMU_BOOT_DISK := chronos.img
QEMU_MAX_RAM := -m 512M
# QEMU_NOX := -nographic

QEMU_OPTIONS := $(QEMU_CPU_COUNT) $(QEMU_MAX_RAM) $(QEMU_NOX) $(QEMU_BOOT_DISK)

.PHONY: qemu qemu-gdb qemu-x qemu-x-gdb

# Standard build and launch recipes
qemu: all
	$(QEMU) -nographic $(QEMU_OPTIONS)
qemu-gdb: all kernel-symbols user-symbols
	$(QEMU) -nographic $(QEMU_OPTIONS) -s -S
qemu-x: all 
	$(QEMU) $(QEMU_OPTIONS)
qemu-x-gdb: all kernel-symbols user-symbols
	$(QEMU) $(QEMU_OPTIONS) -s -S

# Launch current build recipes
run:
	$(QEMU) -nographic $(QEMU_OPTIONS)
run-x:
	$(QEMU) $(QEMU_OPTIONS)
run-gdb: kernel-symbols user/bin $(USER_BUILD) user-symbols
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

.PHONY: clean
clean: 
	cd tools ; \
	make tools-clean
	cd kernel ; \
	make kernel-clean
	rm -rf $(KERNEL_CLEAN) $(TOOLS_CLEAN) $(LIBS_CLEAN) $(USER_CLEAN) fs fs.img chronos.img $(USER_LIB_CLEAN) .bochsrc bochsout.txt chronos.vdi tmp ext2.img
