#include <string.h>

#include "stdlock.h"
#include "fsman.h"
#include "devman.h"
#include "panic.h"
#include "diskio.h"
#include "diskcache.h"
#include "drivers/ext2.h"
#include "drivers/lwfs.h"
#include "drivers/ata.h"

/* Global inode table */
extern slock_t itable_lock;
extern struct inode_t itable[];

/* File system table */
extern slock_t fstable_lock;
extern struct FSDriver fstable[];

static struct FSDriver* fs_alloc(void)
{
        slock_acquire(&fstable_lock);
        struct FSDriver* driver = NULL;
        int x;
        for(x = 0;x < FS_TABLE_MAX;x++)
        {
                if(!fstable[x].valid)
                {
#ifdef CACHE_WATCH
                        cprintf("FSDriver allocated: %d\n", x);
#endif
                        driver = fstable + x;
                        driver->valid = 1;
                        break;
                }
        }

#ifdef CACHE_PANIC
        if(!driver) panic("Too many file systems mounted!\n");
#endif

        slock_release(&fstable_lock);
        return driver;
}

static void fs_free(struct FSDriver* driver)
{
        slock_acquire(&fstable_lock);
        driver->valid = 0;
        slock_release(&fstable_lock);
}

void fsman_init(void)
{
	int x;

	/* Initilize inode table */
	memset(&itable, 0, sizeof(struct inode_t) * FS_INODE_MAX);

	/* Bring up ata drivers */
	//ata_init(); this is now done in devman.

	/* Get a running driver */
	struct FSHardwareDriver* ata = NULL;
	for(x = 0;x < ATA_DRIVER_COUNT;x++)
		if(ata_drivers[x]->valid)
		{
			ata = ata_drivers[x];
			break;
		}
	if(ata == NULL) panic("No ata driver available.\n");

	/* Assign a root file system */
	struct FSDriver* ext2 = fs_alloc();
	ext2->driver = ata;
	diskio_setup(ext2);
	ext2->start = 2048;
	disk_cache_init(ext2);
	
	/* Set our ext2 as the root file system. */
	ext2_init(ext2);
	// ext2->fsck(ext2->context);
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
			return;
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
