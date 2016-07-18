/**
 * Author: Max Strange <mbstrange@wisc.edu>
 * 
 * Maintainers:
 *  + John Detter <john@detter.com>
 *
 * ATA Programmed IO mode driver.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "stdlock.h"
#include "file.h"
#include "fsman.h"
#include "devman.h"
#include "drivers/ata.h"
#include "x86.h"
#include "panic.h"
#include "stdarg.h"
#include "fsman.h"
#include "storagecache.h"

#define PRIMARY_ATA_BASE 0x1F0 /* Base port for primary controller */
#define SECONDARY_ATA_BASE 0x170 /* Base port for secondary controller */

#define ATA_DATA 0x000 /* Read/Write pio data from this port */
#define ATA_FEATURES_ERROR 0x001 /* usually used for ATAPI*/
#define ATA_SECTOR_COUNT 0x002 /* Number of sectors to read/write*/
#define ATA_SECTOR_NUMBER 0x003 /* specific to mode */
#define ATA_CYLINDER_LOW 0x004 /* Partial sectory address */
#define ATA_CYLINDER_HIGH 0x005 /* Partial sector address */
#define ATA_DRIVE 0x006 /* Used to select drive mode */
#define ATA_COMMAND 0x007 /* Command + Status port */

#define SECTSIZE 512
#define ATA_ERR (0x1 << 0) /* An error occurred. Send new command. */
#define ATA_DRQ (0x1 << 3) /* Set when the drive has pio data to transfer. */
#define ATA_SRV (0x1 << 4) /* Overlapped mode service request */
#define ATA_DF (0x1 << 5) /* Drive Fault! */
#define ATA_RDY (0x1 << 6) /* Bit is clear when drive is spun down. */
#define ATA_BSY (0x1 << 7) /* Drive is preparing to send/recieve data. */

#define ATA_NO_DISKS 0xFF /* returned when there are no disks attached. */
#define ATA_NIEN (0x01 << 1)/* Set this to stop interrupts on this disk. */
#define ATA_SRST (0x01 << 2)/* Software reset command.  */
#define ATA_HOB (0x01 << 7)/* set thsi to read back the high order byte. */

#define ATA_IDENTIFY 0xEC

#define ATA_SELECT_MASTER (~(0x01 << 4))
#define ATA_SELECT_SLAVE (0x01 << 4)

static uchar ata_initilized = 0;
static slock_t primary_lock;
static uchar primary_mode; /* 1 = primary, 0 = secondary */
static slock_t secondary_lock;
static uchar secondary_mode; /* 1 = primary, 0 = secondary */

struct ATADriverContext
{
	uint base_port; /* starting port */
	uint master; /* 1 = master, 0 = slave */
	slock_t* lock; /* Pointer to the lock for this context */
	uchar* mode; /* Pointer to the current mode for this context. */
};

static struct StorageDevice ata_primary_master;
static struct StorageDevice ata_primary_slave;
static struct StorageDevice ata_secondary_master;
static struct StorageDevice ata_secondary_slave;

#define ATA_DRIVER_PRIMARY_MASTER 0x00
#define ATA_DRIVER_PRIMARY_SLAVE 0x01
#define ATA_DRIVER_SECONDARY_MASTER 0x02
#define ATA_DRIVER_SECONDARY_SLAVE 0x03
static struct ATADriverContext contexts[ATA_DRIVER_COUNT];
struct StorageDevice* ata_drivers[ATA_DRIVER_COUNT] = 
{&ata_primary_master, &ata_primary_slave, 
	&ata_secondary_master, &ata_secondary_slave};

static int ata_attached(struct StorageDevice* driver);
static int ata_readsect(sect_t sect, void* dst, size_t sz, void* c);
static int ata_writesect(sect_t sect, void* src, size_t sz, void* c);

void ata_init(void)
{
	/* Do not initilize ata more than once. */
	if(ata_initilized) return;
	ata_initilized = 1;

	/* Verify context size */
	if(sizeof(struct ATADriverContext) > FS_HARDWARE_CONTEXT_SIZE)
		panic("ATA driver context is too large.");

	slock_init(&primary_lock);
	slock_init(&secondary_lock);

	/* Both controllers start in master mode */
	primary_mode = 1;
	secondary_mode = 1;

	/* setup all drivers */
	int x;
	for(x = 0;x < ATA_DRIVER_COUNT;x++)
	{
		memset(ata_drivers[x], 0, sizeof(struct StorageDevice));
		ata_drivers[x]->readsect = ata_readsect;
		ata_drivers[x]->writesect = ata_writesect;
		/* Assign a context */
		struct ATADriverContext* context = contexts + x;
		ata_drivers[x]->context = context;
		ata_drivers[x]->sectsize = 512;
		/* TODO: Get the sector size */
		ata_drivers[x]->sectshifter = 9;
		/* TODO: calculate the last sector */
		ata_drivers[x]->sectors = (uint)-1;
		ata_drivers[x]->spp = PGSIZE / ata_drivers[x]->sectsize;

		switch(x)
		{
			case ATA_DRIVER_PRIMARY_MASTER:
				context->base_port = PRIMARY_ATA_BASE;
				context->master = 1;
				context->mode = &primary_mode;
				context->lock = &primary_lock;
				break;
			case ATA_DRIVER_PRIMARY_SLAVE:
				context->base_port = PRIMARY_ATA_BASE;
				context->master = 0;
				context->mode = &primary_mode;
				context->lock = &primary_lock;
				break;
			case ATA_DRIVER_SECONDARY_MASTER:
				context->base_port = SECONDARY_ATA_BASE;
				context->master = 1;
				context->mode = &secondary_mode;
				context->lock = &secondary_lock;
				break;
			case ATA_DRIVER_SECONDARY_SLAVE:
				context->base_port = SECONDARY_ATA_BASE;
				context->master = 0;
				context->mode = &secondary_mode;
				context->lock = &secondary_lock;
				break;
		}

		/* See if a disk is attached to the controller */
		if(ata_attached(ata_drivers[x]))
			ata_drivers[x]->valid = 1;
	}
}

static void ata_set_mode(uchar mode, struct StorageDevice* driver)
{
	struct ATADriverContext* context = driver->context;
	uint port = context->base_port;
	uchar* curr_mode = context->mode;
	if(*curr_mode == mode) return;

	uchar curr_flags = inb(port + ATA_DRIVE);
	/* Adjust for the mode */
	if(mode) curr_flags &= ATA_SELECT_MASTER;
	else curr_flags |= ATA_SELECT_SLAVE;

	outb(port + ATA_DRIVE, curr_flags);
	*curr_mode = mode;

	/* If the mode has changed, we must wait 400ns */
	int x;
	for(x = 0;x < 4;x++)
		inb(port + ATA_COMMAND);
}

static int ata_attached(struct StorageDevice* driver)
{
	struct ATADriverContext* context = driver->context;
	uint port = context->base_port;
	/* Make sure were in the proper mode */
	ata_set_mode(context->master, driver);	

	uchar status = inb(port + ATA_COMMAND);
	if(status == 0xFF || status == 0x00) return 0;
	return 1;
}

void ata_wait(struct StorageDevice* driver)
{
	struct ATADriverContext* context = driver->context;
	while((inb(context->base_port + ATA_COMMAND) 
				& (ATA_RDY | ATA_BSY)) != ATA_RDY);
}

static int ata_readsect(sect_t sect, void* dst, size_t sz, void* c)
{
	struct StorageDevice* driver = c;
	struct ATADriverContext* context = driver->context;
	if(sz < driver->sectsize)
		return -1;

	uchar m = 0xE0;
	if(!context->master) m = 0xF0;

	outb(context->base_port + ATA_DRIVE, m | ((sect >> 24) & 0x0F));
	outb(context->base_port + ATA_FEATURES_ERROR, 0x0); /* waste */
	outb(context->base_port + ATA_SECTOR_COUNT, 0x1);
	outb(context->base_port + ATA_SECTOR_NUMBER, sect);
	outb(context->base_port + ATA_CYLINDER_LOW, sect >> 8);
	outb(context->base_port + ATA_CYLINDER_HIGH, sect >> 16);
	//outb(context->base_port + ATA_CYLINDER_HIGH, (sect >> 24) & 0xE0);
	outb(context->base_port + ATA_COMMAND, 0x20);

	ata_wait(driver);
	insl(context->base_port + ATA_DATA, dst, SECTSIZE/4);

	return 0;
}

static int ata_writesect(sect_t sect, void* src, size_t sz, void* c)
{
	struct StorageDevice* driver = c;
	struct ATADriverContext* context = driver->context;
	if(sz < driver->sectsize)
		return -1;

	uchar m = 0xE0;
	if(!context->master) m = 0xF0;
	outb(context->base_port + ATA_DRIVE, m | ((sect >> 24) & 0x0F));
	outb(context->base_port + ATA_SECTOR_COUNT, 0x1);
	outb(context->base_port + ATA_SECTOR_NUMBER, sect);
	outb(context->base_port + ATA_CYLINDER_LOW, sect >> 8);
	outb(context->base_port + ATA_CYLINDER_HIGH, sect >> 16);
	//outb(context->base_port + ATA_CYLINDER_HIGH, (sect >> 24) & 0xE0);
	outb(context->base_port + ATA_COMMAND, 0x30);

	ata_wait(driver);
	uint* srcw = (uint*)src;
	outsl(context->base_port + ATA_DATA, srcw, SECTSIZE/4);

	/* Cache flush*/
	outb(context->base_port + ATA_COMMAND, 0xE7);
	ata_wait(driver);

	return 0;
}

/** ATA devices are io devices so they must define IODevice methods. */
static int ata_io_init(struct IODevice* device);
static int ata_io_read(void* dst, uint start_read, size_t sz,
		struct StorageDevice* context);
static int ata_io_write(void* src, uint start_write, size_t sz, 
		struct StorageDevice* context);
int ata_io_setup(struct IODevice* device, struct StorageDevice* ata)
{
	device->context = ata;
	device->init = ata_io_init;
	device->read = (void*)ata_io_read;
	device->write = (void*)ata_io_write;
	return 0;
}

static int ata_io_init(struct IODevice* device)
{
	return 0;
}

static int ata_io_read(void* dst, sect_t start_read, size_t sz, 
		struct StorageDevice* context)
{
	sect_t sect_start = start_read / SECTSIZE;
	sect_t sect_end = (start_read + sz - 1) / SECTSIZE;
	size_t bytes_read = 0;
	char sector[SECTSIZE];

	if(sect_start == sect_end)
	{
		if(ata_readsect(sect_start, sector, SECTSIZE, context))
			return -1;
		memmove(dst, sector + (start_read % SECTSIZE), sz);
		return sz;
	}

	for(bytes_read = 0;bytes_read < sz;)
	{
		int sect = (start_read + bytes_read) / SECTSIZE;
		int bytes = 0;
		if(ata_readsect(sect, sector, SECTSIZE, context))
			return -1;

		if(sect == sect_start)
		{
			bytes = SECTSIZE - (start_read % SECTSIZE);
			memmove(dst, sector + (start_read % SECTSIZE), bytes);
		} else if(sect == sect_end)
		{
			bytes = sz - bytes_read;
			memmove(dst + bytes_read, sector,
					sz - bytes_read);
		} else {
			bytes = SECTSIZE;
			memmove(dst + bytes_read, sector, SECTSIZE);
		}

		bytes_read += bytes;
	}

	return sz;
}

static int ata_io_write(void* src, uint start_write, size_t sz,
		struct StorageDevice* context)
{
	sect_t sect_start = start_write / SECTSIZE;
	sect_t sect_end = (start_write + sz - 1) / SECTSIZE;
	size_t bytes_written = 0;
	char sector[SECTSIZE];

	if(sect_start == sect_end)
	{
		if(ata_readsect(sect_start, sector, SECTSIZE, context))
			return -1;
		memmove(sector + (start_write % SECTSIZE), src, sz);
		if(ata_writesect(sect_start, sector, SECTSIZE, context))
			return -1;
		return sz;
	} 

	for(bytes_written = 0;bytes_written < sz;)
	{
		int sect = (start_write + bytes_written) / SECTSIZE;
		int bytes = 0;
		if(ata_readsect(sect, sector, SECTSIZE, context))
			return -1;
		if(sect == sect_start)
		{
			bytes = SECTSIZE - (start_write % SECTSIZE);
			memmove(sector + (start_write % SECTSIZE), src, bytes);
		} else if(sect == sect_end)
		{
			bytes = sz - bytes_written;
			memmove(sector, src + bytes_written, sz-bytes_written);
		} else {
			bytes = SECTSIZE;
			memmove(sector, src + bytes_written, SECTSIZE);
		}

		if(ata_writesect(sect, sector, SECTSIZE, context))
			return -1;
		bytes_written += bytes;
	}

	return sz;
}
