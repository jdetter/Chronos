#include <string.h>
#include "storageio.h"
#include "panic.h"
#include "storagecache.h"
#include "drivers/ata.h"
#include "drivers/ext2.h"
#include "drivers/lwfs.h"

extern struct FSDriver* fs_alloc(void);
extern void fs_free(struct FSDriver* driver);

extern slock_t itable_lock;
extern struct inode_t itable[];

void fsman_init(void)
{
	/* Initilize inode table */
	memset(&itable, 0, sizeof(struct inode_t) * FS_INODE_MAX);

	/* Get a running driver */
	struct StorageDevice* ata = NULL;
	int x;
	for(x = 0;x < ATA_DRIVER_COUNT;x++)
	{
		if(ata_drivers[x]->valid)
		{
			ata = ata_drivers[x];
			break;
		}
	}
	if(ata == NULL) panic("No ata driver available.\n");

	/* Assign a root file system */
	struct FSDriver* ext2 = fs_alloc();
	ext2->driver = ata;
	ext2->fs_start = 2048;
	storage_cache_init(ext2);

	/* Set our ext2 as the root file system. */
	ext2_init(ext2);
	ext2->valid = 1;
	ext2->type = 0;
	strncpy(ext2->mount_point, "/", FILE_MAX_PATH);

	/* If the directory is already there, delete it */
	ext2->rmdir("/dev", ext2->context);        

	/* make sure there is a dev folder */
	if(ext2->mkdir("/dev",
				PERM_ARD | PERM_AEX | PERM_UWR | PERM_GWR | S_IFDIR,
				0x0, 0x0, ext2->context))
	{
		/* If this fails, then there must be junk in the directory */
		/* Make sure the permissions are okay at least */
		void* ino = ext2->open("/dev", ext2->context);
		if(!ino)
		{
			/* If we can't open dev the file system is currupt */
			panic("EXT2 files system currupt.");
		}

		ext2->chmod(ino,
				PERM_ARD | PERM_AEX | PERM_UWR
				| PERM_GWR | S_IFDIR, ext2->context);
		ext2->chown(ino, 0, 0, ext2->context);
	}

	/* Create a light weight file system on /dev */
	struct FSDriver* dev = fs_alloc();
	if(lwfs_init(0x100000, dev))
	{
		cprintf("kernel: failed to create /dev ram file system.\n");
		fs_free(dev);
		return;
	}

	dev->type = 0;
	dev->valid = 1;
	strncpy(dev->mount_point, "/dev/", FILE_MAX_PATH);
}
