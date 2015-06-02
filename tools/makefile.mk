TOOLS := boot-sign \
	mkfs \
	disk-part \
	fsck \
	mkvect

TOOLS_BINARIES := $(addprefix tools/bin/, $(TOOLS))

TOOLS_CLEAN := tools/bin/

tools: tools-dir $(TOOLS_BINARIES)

tools-dir:
	mkdir -p tools/bin/

tools/bin/mkfs:
# VSFS must be rebuilt in MKFS mode
	echo "#define VSFS_MKFS" > tools/bin/vsfs.c
	cat kernel/drivers/vsfs.c >> tools/bin/vsfs.c
	cp include/types.h tools/bin
	$(CC) $(CFLAGS) -I kernel/drivers/ -c -o tools/bin/vsfs.o tools/bin/vsfs.c
	$(CC) $(CFLAGS) -I kernel/drivers/ -I tools/bin -o tools/bin/mkfs tools/mkfs.c tools/bin/vsfs.o
	
tools/bin/fsck: tools/bin/mkfs
	$(CC) $(CFLAGS) -I kernel/drivers/ -I tools/bin -o tools/bin/fsck tools/fsck.c tools/bin/vsfs.o

tools/bin/boot-sign:
	$(CC) $(CFLAGS) -o tools/bin/boot-sign tools/boot-sign.c
tools/bin/disk-part:
	$(CC) $(CFLAGS) -o tools/bin/disk-part tools/disk-part.c
tools/bin/mkvect:
	$(CC) $(CFLAGS) -o tools/bin/mkvect tools/mkvect.c -I kernel -I include
