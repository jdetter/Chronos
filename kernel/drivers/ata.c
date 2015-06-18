#include "types.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"
#include "ata.h"
#include "x86.h"
#include "panic.h"
#include "stdarg.h"
#include "stdlib.h"

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

static struct FSHardwareDriver ata_primary_master;
static struct FSHardwareDriver ata_primary_slave;
static struct FSHardwareDriver ata_secondary_master;
static struct FSHardwareDriver ata_secondary_slave;

#define ATA_DRIVER_PRIMARY_MASTER 0x00
#define ATA_DRIVER_PRIMARY_SLAVE 0x01
#define ATA_DRIVER_SECONDARY_MASTER 0x02
#define ATA_DRIVER_SECONDARY_SLAVE 0x03
#define ATA_DRIVER_COUNT 4
static struct ATADriverContext contexts[ATA_DRIVER_COUNT];
struct FSHardwareDriver* ata_drivers[ATA_DRIVER_COUNT] = 
	{&ata_primary_master, &ata_primary_slave, 
	&ata_secondary_master, &ata_secondary_slave};

int ata_attached(struct FSHardwareDriver* driver);

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
		memset(ata_drivers[x], 0, sizeof(struct FSHardwareDriver));
		ata_drivers[x]->read = ata_readsect;
		ata_drivers[x]->write = ata_writesect;
		/* Assign a context */
		struct ATADriverContext* context = contexts + x;
		ata_drivers[x]->context = context;

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

void ata_set_mode(uchar mode, struct FSHardwareDriver* driver)
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

int ata_attached(struct FSHardwareDriver* driver)
{
	struct ATADriverContext* context = driver->context;
	uint port = context->base_port;
	/* Make sure were in the proper mode */
	ata_set_mode(context->master, driver);	

	uchar status = inb(port + ATA_COMMAND);
	if(status == 0xFF || status == 0x00) return 0;
	return 1;
}

void ata_wait(struct FSHardwareDriver* driver)
{
	struct ATADriverContext* context = driver->context;
	while((inb(context->base_port + ATA_COMMAND) 
		& (ATA_RDY | ATA_BSY)) != ATA_RDY);
}

int ata_readsect(void* dst, uint sect, struct FSHardwareDriver* driver)
{
	struct ATADriverContext* context = driver->context;
	ata_set_mode(context->master, driver);
	ata_wait(driver);
	outb(context->base_port + ATA_SECTOR_COUNT, 0x1);
	outb(context->base_port + ATA_SECTOR_NUMBER, sect);
	outb(context->base_port + ATA_CYLINDER_LOW, sect >> 8);
	outb(context->base_port + ATA_CYLINDER_HIGH, sect >> 16);
	outb(context->base_port + ATA_CYLINDER_HIGH, (sect >> 24) & 0xE0);
	outb(context->base_port + ATA_COMMAND, 0x20);

	ata_wait(driver);
	insl(context->base_port + ATA_DATA, dst, SECTSIZE/4);

	return 0;
}

int ata_writesect(void* src, uint sect, struct FSHardwareDriver* driver)
{
	struct ATADriverContext* context = driver->context;
	outb(ATA_SECTOR_COUNT, 0x1);
	outb(ATA_SECTOR_NUMBER, sect);
	outb(ATA_CYLINDER_LOW, sect >> 8);
	outb(ATA_CYLINDER_HIGH, sect >> 16);
	outb(ATA_CYLINDER_HIGH, (sect >> 24) & 0xE0);
	outb(ATA_COMMAND, 0x30);

	ata_wait(driver);

	uint* srcw = (uint*)src;

	outsl(context->base_port + ATA_DATA, srcw, 512/4);

	outb(context->base_port + ATA_COMMAND, 0xE7);
	ata_wait(driver);

	return 0;
}
