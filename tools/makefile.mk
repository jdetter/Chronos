TOOLS := boot-sign \
	mkfs \
	disk-part \
	fsck \
	boot2-verify \
	mkvect

TOOLS_BINARIES := $(addprefix tools/bin/, $(TOOLS))
TOOLS_CLEAN := tools/bin/

TOOLS_BUILD := tools/bin $(TOOLS_BINARIES)

.PHONY: tools
tools: $(TOOLS_BUILD)

tools/bin:
	mkdir -p tools/bin/

tools/bin/mkfs: tools/bin lib/file.o
# VSFS must be rebuilt in MKFS mode
	echo "#define VSFS_MKFS" > tools/bin/vsfs.c
	cat kernel/drivers/vsfs.c >> tools/bin/vsfs.c
	cp include/stdlock.h tools/bin
	cp include/types.h tools/bin
	cp include/file.h tools/bin
	$(CC) $(CFLAGS) -I kernel/drivers/ -I kernel/ -I tools/bin -c -o tools/bin/vsfs.o tools/bin/vsfs.c
	$(CC) $(CFLAGS) -I kernel/drivers/ -I kernel/ -I tools/bin -o tools/bin/mkfs tools/mkfs.c tools/bin/vsfs.o lib/file.o
	
tools/bin/fsck: tools/bin/mkfs lib/file.o
	$(CC) $(CFLAGS) -I kernel/drivers/ -I tools/bin -I kernel/ -o tools/bin/fsck tools/fsck.c tools/bin/vsfs.o lib/file.o

tools/bin/boot2-verify: tools/bin
	$(CC) $(CFLAGS) -o tools/bin/boot2-verify tools/boot2-verify.c
tools/bin/boot-sign: tools/bin
	$(CC) $(CFLAGS) -o tools/bin/boot-sign tools/boot-sign.c
tools/bin/disk-part: tools/bin
	$(CC) $(CFLAGS) -o tools/bin/disk-part tools/disk-part.c
tools/bin/mkvect: tools/bin
	$(CC) $(CFLAGS) -o tools/bin/mkvect tools/mkvect.c -I kernel -I tools/bin
