#ifndef _ATA_H_
#define _ATA_H_

/* Amount of possible ata drivers */
#define ATA_DRIVER_COUNT 4
/**
 * Hardware context. Can be used to represent the 4 types of ata_pio
 * hard drives.
 */
struct ATAHardwareContext
{
        uchar primary; /* 1 = primary, 0 = secondary */
        uchar master; /* 1 = master, 0 = slave*/
};

/**
 * Initilize the ata hardware drivers.
 */
void ata_init(void);

/**
 * Read a hard drive sector into the destination buffer, dst. Returns the
 * amount of bytes read from the disk. If there was an error return 0.
 * Note: sector sizes will always be treated as 512 byte buffers, so the
 * dst buffer is expected to be at least 512 bytes.
 */
int ata_readsect(void* dst, uint sect, struct FSHardwareDriver* driver);

/**
 * Write the bytes in src to a hard drive sector. Returns the amount of bytes
 * written to the disk. If there was an error, returns 0.
 * Note: sector sizes will always be treated as 512 byte buffers, so the
 * src buffer is expected to be at least 512 bytes. 
 */
int ata_writesect(void* src, uint sect, struct FSHardwareDriver* driver);

/**
 * For setting up generic io driver.
 */
int ata_io_setup(struct IODriver* driver, struct FSHardwareDriver* ata);

#define ATA_DRIVER_COUNT 4
extern struct FSHardwareDriver* ata_drivers[];
#endif
