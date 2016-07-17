/**
 * Author: John Detter <john@detter.com>
 *
 * This is Chronos's device manager. Devman detects devices on boot and
 * assigns them drivers and nodes on /dev.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "kstdlib.h"
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
#include "cacheman.h"
#include "storagecache.h"
#include "cpu.h"
#include "vm.h"
#include "k/netman.h"
#include "device.h"
#include "drivers/ata.h"
#include "drivers/pic.h"
#include "drivers/fpu.h"
#include "drivers/cmos.h"
#include "drivers/pit.h"
#include "k/drivers/keyboard.h"
#include "k/drivers/serial.h"
#include "k/drivers/console.h"

extern int video_mode;
extern uchar serial_connected;

int dev_init()
{
	/* Enable PIC */
	cprintf("Starting Programmable Interrupt Controller Driver...\t\t\t");
	pic_init();
	cprintf("[ OK ]\n");

	/* Enable the floating point unit */
	cprintf("Enabling the floating point unit.\t\t\t\t\t");
	fpu_init();
	cprintf("[ OK ]\n");

	/* Initilize CMOS */
	cprintf("Initilizing cmos...\t\t\t\t\t\t\t");
	cmos_init();
	cprintf("[ OK ]\n");

	/* Start the network manager */
	cprintf("Starting network manager...\t\t\t\t\t\t");
	net_init();
	cprintf("[ OK ]\n");

	/* Enable PIT */
	cprintf("Starting Programmable Interrupt Timer Driver...\t\t\t\t");
	pit_init();
	cprintf("[ OK ]\n");

	/* Initilize cli stack */
	cprintf("Initilizing cli stack...\t\t\t\t\t\t");
	reset_cli();
	cprintf("[ OK ]\n");

	struct IODevice* device = NULL;

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
	int x;
	for(x = 0;;x++)
	{
		if(x > 0 && !video_type) break;
		tty_t t = tty_find(x);
		if(t == NULL) break;
		device = dev_alloc();
		if(device == NULL)
			panic("Out of free device drivers.\n");

		/* Valid tty */
		if(x != 0)
			tty_init(t, x, video_type, 1, video_mem);
		tty_io_setup(device, x);
		t->driver = device;
		tty_io_setup(device, x);

		/* Set mount point */
		device->type = DEV_TTY;
		snprintf(device->node, 
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
		device = dev_alloc();
		device->type = DEV_DISK;
		ata_io_setup(device, ata_drivers[x]);
		snprintf(device->node,
				FILE_MAX_PATH, "/dev/hd%c", 'a' + x);

		void* disk_cache = cman_alloc(ATA_CACHE_SZ);
		/* Initilize the cache for this disk */
		if(cache_init(disk_cache, ATA_CACHE_SZ, PGSIZE, 
					"", &ata_drivers[x]->cache))
			panic("Cache init for disk failed!\n");
		/* Set a real name for the cache */
		snprintf(ata_drivers[x]->cache.name, CACHE_DEBUG_NAME_LEN,
				"ATA DRIVE %d", (x + 1));
		storage_cache_hardware_init(ata_drivers[x]);
	}

	serial_init(0);
	/* Find serial port */
	if(serial_connected)
	{
		device = dev_alloc();
		serial_io_setup(device);
		/* Add serial to mask */
		pic_enable(INT_PIC_COM1);
		/* Set mount point */
		device->type = DEV_COM;
		snprintf(device->node,
				FILE_MAX_PATH, "/dev/sl0");
	}

	/* Keyboard */
	kbd_init();

	/* Enable events from keyboards */
	tty_setup_kbd_events();

	return 0;
}
