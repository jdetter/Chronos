/**
 * Author: John Detter <john@detter.com>
 *
 * This is Chronos's device manager. Devman detects devices on boot and
 * assigns them drivers and nodes on /dev.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "kern/types.h"
#include "kern/stdlib.h"
#include "x86.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "stdarg.h"
#include "stdlib.h"
#include "panic.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "cacheman.h"
#include "diskcache.h"
#include "device.h"
#include "drivers/ata.h"
#include "drivers/pic.h"
#include "drivers/keyboard.h"
#include "drivers/console.h"
#include "drivers/serial.h"

extern pgdir_t* k_pgdir;
extern uint video_mode;
slock_t driver_table_lock;
static struct DeviceDriver drivers[MAX_DEVICES];
extern uchar serial_connected;
uint curr_mapping;

struct DeviceDriver* dev_null; /* null device driver */
struct DeviceDriver* dev_zero; /* zero device driver */

int io_null_init(struct IODriver* driver);
int io_zero_init(struct IODriver* driver);

static struct DeviceDriver* dev_alloc(void)
{
	slock_acquire(&driver_table_lock);
	struct DeviceDriver* driver = NULL;
	int x;
	for(x = 1;x < MAX_DEVICES;x++)
		if(drivers[x].valid == 0)
		{
			driver = drivers + x;
			driver->valid = 1;
			break;
		}
	slock_release(&driver_table_lock);
	return driver;
}

void* dev_new_mapping(uint phy, uint sz)
{
	vmflags_t dir_flags = VM_DIR_READ | VM_DIR_WRIT;
	vmflags_t tbl_flags = VM_TBL_READ | VM_TBL_WRIT 
		| VM_TBL_CACH | VM_TBL_WRTH;

	uint v_start = curr_mapping;
	uint v_end = PGROUNDUP(v_start + sz);
	if(v_end > KVM_HARDWARE_E)
		panic("devman: Out of hardware memory.\n");
	uint x;
	for(x = 0;x < sz;x += PGSIZE)
		vm_mappage(phy + x, v_start + x, k_pgdir, 
			dir_flags, tbl_flags);

	curr_mapping = v_end;
	return (void*)v_start;
}

int dev_init()
{
	curr_mapping = KVM_HARDWARE_S;
	slock_init(&driver_table_lock);
	struct DeviceDriver* driver = NULL;

	memset(&drivers, 0, sizeof(struct DeviceDriver) * MAX_DEVICES);

	/* Setup video memory */
	uint video_mem = 0x0;
	uchar video_type = 0;
	if(video_mode == VIDEO_MODE_COLOR)
	{
		video_type = TTY_TYPE_COLOR;
		video_mem = CONSOLE_COLOR_BASE_ORIG;
	}else if(video_mode == VIDEO_MODE_MONO)
	{
		video_type = TTY_TYPE_MONO;
		video_mem = CONSOLE_MONO_BASE_ORIG;
	} else {
		video_type = 0x0;
		video_mem = 0x0;
	}

	if(video_mem)
		video_mem = (uint)dev_new_mapping(video_mem, 
				CONSOLE_MEM_SZ);

	/* Find ttys */
	uint x;
	for(x = 0;;x++)
	{
		if(x > 0 && !video_type) break;
		tty_t t = tty_find(x);
		if(t == NULL) break;
		driver = dev_alloc();
		if(driver == NULL)
			panic("Out of free device drivers.\n");
		/* Valid tty */
		if(x != 0)
			tty_init(t, x, video_type, 1, video_mem);
		tty_io_setup(&driver->io_driver, x);
		t->driver = driver;
		tty_io_setup(&driver->io_driver, x);
		/* Set mount point */
		driver->type = DEV_TTY;
		snprintf(driver->node, 
			FILE_MAX_PATH, "/dev/tty%d", x);
	}

	/* Fix tty0 */
	tty_t t0 = tty_find(0);
	if(t0->type == TTY_TYPE_COLOR || t0->type == TTY_TYPE_MONO)
		t0->mem_start = video_mem;
	/* Boot strap has directly mapped the video memory */
	vm_unmappage(CONSOLE_MONO_BASE_ORIG, k_pgdir);
	vm_unmappage(CONSOLE_COLOR_BASE_ORIG, k_pgdir);

	/* Find ata devices */
	ata_init();

	for(x = 0;x < ATA_DRIVER_COUNT;x++)
	{
		if(ata_drivers[x]->valid != 1) continue;
		/* Valid ata driver */
		driver = dev_alloc();
		driver->type = DEV_DISK;
		ata_io_setup(&driver->io_driver, ata_drivers[x]);
		snprintf(driver->node,
			FILE_MAX_PATH, "/dev/hd%c", 'a' + x);

		void* disk_cache = cman_alloc(ATA_CACHE_SZ);
		/* Initilize the cache for this disk */
		if(cache_init(disk_cache, ATA_CACHE_SZ, PGSIZE, 
			"", &ata_drivers[x]->cache))
			cprintf("Cache init for disk failed!\n");
		/* Set a real name for the cache */
		snprintf(ata_drivers[x]->cache.name, CACHE_DEBUG_NAME_LEN,
			"ATA DRIVE %d", (x + 1));
		disk_cache_hardware_init(ata_drivers[x]);
	}


	serial_init(0);
	/* Find serial port */
	if(serial_connected)
	{
		driver = dev_alloc();
		serial_io_setup(&driver->io_driver);
		/* Add serial to mask */
		pic_enable(INT_PIC_COM1);
		/* Set mount point */
		driver->type = DEV_COM;
                snprintf(driver->node,
                        FILE_MAX_PATH, "/dev/sl0");
	}

	/* Keyboard */
	kbd_init();

	/* Enable events from keyboards */
	tty_setup_kbd_events();

	/* make null and zero devices */
	driver = dev_alloc();
	driver->type = DEV_IO;
	snprintf(driver->node, FILE_MAX_PATH, "/dev/null");
	driver->io_driver.init = io_null_init;
	dev_null = driver;

	driver = dev_alloc();
        driver->type = DEV_IO;
        snprintf(driver->node, FILE_MAX_PATH, "/dev/zero");
        driver->io_driver.init = io_zero_init;
	dev_zero = driver;

	/* Do final init on all io devices */
	for(x = 0;x < MAX_DEVICES;x++)
		if(drivers[x].valid)
			drivers[x].io_driver.init(&drivers[x].io_driver);
	return 0;
}

void dev_populate(void)
{
	int dev_perm = PERM_URD | PERM_UWR |
		PERM_GRD | PERM_GWR | 
		PERM_ORD | PERM_OWR;
	dev_t x;
	for(x = 0;x < MAX_DEVICES;x++)
	{
		if(!drivers[x].valid) continue;
		dev_t type = drivers[x].type;
		switch(type)
		{
			/* Character devices */
			case DEV_IO:
			case DEV_TTY:
			case DEV_COM:
				type = S_IFCHR;
				break;
			/* Block devices */
			case DEV_DISK_PART:
			case DEV_DISK:
			case DEV_RAM:
			case DEV_LOOP:
				type = S_IFBLK;
				break;
			case DEV_PIPE:
				type = S_IFIFO;
				break;
			case DEV_SOCK:
				type = S_IFSOCK;
				break;
			default:
				type = 0;
				cprintf("Invalid device: %d\n", x);
				break;
		}
		if(!type) continue;

		if(drivers[x].valid)
		{
			fs_mknod(drivers[x].node, x, drivers[x].type, 
				dev_perm | type);
		}
	}
}

struct DeviceDriver* dev_lookup(int dev)
{
	if(dev > 0 && dev < MAX_DEVICES && drivers[dev].valid)
		return drivers + dev;
	else return NULL;
}

int io_null_read(void* dst, uint start_read, size_t sz, void* context)
{
	return 0;
}

int io_null_write(void* dst, uint start_read, size_t sz, void* context)
{
	return sz;
}

int io_null_init(struct IODriver* driver)
{
	driver->init = io_null_init;
	driver->read = io_null_read;
	driver->write = io_null_write;
	driver->ioctl = NULL;
	return 0;
}

int io_zero_read(void* dst, uint start_read, size_t sz, void* context)
{
	memset(dst, 0, sz);
        return sz;
}

int io_zero_write(void* dst, uint start_read, size_t sz, void* context)
{
        return sz;
}

int io_zero_init(struct IODriver* driver)
{
        driver->init = io_zero_init;
        driver->read = io_zero_read;
        driver->write = io_zero_write;
        driver->ioctl = NULL;
        return 0;
}
