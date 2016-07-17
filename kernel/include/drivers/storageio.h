#ifndef _DISKIO_H_
#define _DISKIO_H_

#include "fsman.h"

int storageio_readsect(sect_t start_sect, void* src, size_t sz, 
		struct StorageDevice* device);
int storageio_readsects(sect_t start_sect, int sectors, void* src, 
		size_t sz, struct StorageDevice* device);
int storageio_writesect(sect_t start_sect, void* src, size_t sz, 
		struct StorageDevice* device);
int storageio_writesects(sect_t start_sect, int sectors, void* src, 
		size_t sz, struct StorageDevice* device);

/**
 * Read from the disk at the byte level. Start reading from start
 * until start + sz. The dst buffer must be large enough to hold
 * sz bytes. The amount of bytes read is returned. -1 is returned
 * on failure.
 */
int storageio_read(void* dst, fileoff_t start, size_t sz,
		struct FSDriver* driver);

/**
 * Write to the disk at the byte level. Start writing from start
 * until start + sz. The src buffer must be at least sz bytes.
 */
int storageio_write(void* src, fileoff_t start, size_t sz, 
		struct FSDriver* driver);

#endif
