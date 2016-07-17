#ifndef _ATA_H_
#define _ATA_H_

#include "fsman.h"
#include "devman.h"

typedef unsigned int sect_t;

/* Amount of possible ata drivers */
#define ATA_DRIVER_COUNT 4
/**
 * Hardware context. Can be used to represent the 4 types of ata_pio
 * hard drives.
 */
struct ATAHardwareContext
{
	int primary; /* 1 = primary, 0 = secondary */
	int master; /* 1 = master, 0 = slave*/
};

/**
 * Initilize the ata hardware drivers.
 */
void ata_init(void);

/**
 * For setting up generic io driver.
 */
int ata_io_setup(struct IODevice* device, struct StorageDevice* ata);

#define ATA_DRIVER_COUNT 4
extern struct StorageDevice* ata_drivers[];
#endif
