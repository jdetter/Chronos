#ifndef _ATA_H_
#define _ATA_H_

/**
 * Polls the hard drive until it is ready for a PIO read/write.
 */
void ata_wait();

/**
 * Read a hard drive sector into the destination buffer, dst. Returns the
 * amount of bytes read from the disk. If there was an error return 0.
 * Note: sector sizes will always be treated as 512 byte buffers, so the
 * dst buffer is expected to be at least 512 bytes.
 */
int ata_readsect(uint sect, char* dst);

/**
 * Write the bytes in src to a hard drive sector. Returns the amount of bytes
 * written to the disk. If there was an error, returns 0.
 * Note: sector sizes will always be treated as 512 byte buffers, so the
 * src buffer is expected to be at least 512 bytes. 
 */
int ata_writesect(uint sect, char* src);
#endif
