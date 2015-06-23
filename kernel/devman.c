#include "types.h"
#include "x86.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "stdarg.h"
#include "stdlib.h"
#include "ata.h"
#include "serial.h"
#include "panic.h"
#include "proc.h"
#include "vm.h"

slock_t io_driver_table_lock;
struct IODriver io_drivers[MAX_DEVICES];
extern uchar serial_connected;

static struct IODriver* dev_alloc(void)
{
	slock_acquire(&io_driver_table_lock);
	struct IODriver* driver = NULL;
	int x;
	for(x = 1;x < MAX_DEVICES;x++)
		if(io_drivers[x].valid == 0)
		{
			driver = io_drivers + x;
			driver->valid = 1;
			break;
		}
	slock_release(&io_driver_table_lock);
	return driver;
}

int dev_init()
{
	slock_init(&io_driver_table_lock);
	io_drivers[0].valid = 0;
	struct IODriver* driver = NULL;

	/* Find ttys */
	uint x;
	for(x = 0;;x++)
	{
		tty_t t = tty_find(x);
		if(t == NULL) break;
		driver = dev_alloc();
		if(driver == NULL)
			panic("Out of free device drivers.\n");
		/* Valid tty */
		if(x != 0)
		tty_init(t, x, TTY_TYPE_COLOR, 1, KVM_COLOR_START);
		tty_io_setup(driver, x);
		/* Set mount point */
		snprintf(driver->node, 
			FILE_MAX_PATH, "/dev/tty%d", x);
	}

	/* Find ata devices */
	ata_init();

	for(x = 0;x < ATA_DRIVER_COUNT;x++)
	{
		if(ata_drivers[x]->valid != 1) continue;
		/* Valid ata driver */
		driver = dev_alloc();
		ata_io_setup(driver, ata_drivers[x]);
	}


	serial_init(INT_PIC_TIMER_CODE);
	/* Find serial port */
	if(serial_connected)
	{
		driver = dev_alloc();
		serial_io_setup(driver);
		/* Set mount point */
                snprintf(driver->node,
                        FILE_MAX_PATH, "/dev/sl0", x);
	}

	/* Do final init on all devices */
	for(x = 0;x < MAX_DEVICES;x++)
		if(io_drivers[x].valid)
			io_drivers[x].init(io_drivers + x);
	return 0;
}


int dev_read(struct IODriver* device, void* dst, uint start_read, uint sz)
{
	if(!device->valid) return -1;
	return device->read(dst, start_read, sz, device->context);
}

int dev_write(struct IODriver* device, void* src, uint start_write, uint sz)
{
	if(!device->valid) return -1;
	return device->write(src, start_write, sz, device->context);
}
