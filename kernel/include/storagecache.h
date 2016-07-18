#ifndef _DISKCACHE_H_
#define _DISKCACHE_H_

#include "devman.h"
#include "fsman.h"

/**
 * Get a reference to a sector on the given storage device.
 * Returns NULL if the sector couldn't be reference.
 */
void* storage_cache_reference_global(sect_t sect, 
		struct StorageDevice* device);

/**
 * Get a reference to a sector on the given storage device. The contents
 * of the sector will not be loaded from disk, only a reference in memory
 * will be created. This is useful for use cases when the entire block is
 * being overwritten.
 */
void* storage_cache_addreference_global(sect_t sect,
		        struct StorageDevice* device);

/**
 * Release a reference to a sector. Returns 0 on success, non zero otherwise.
 */
int storage_cache_dereference_global(void* ref,
		        struct StorageDevice* device);

/**
 * Setup an FSDriver for using the standard cache functions.
 */
int storage_cache_init(struct FSDriver* driver);

/**
 * Initilize a storage device with cache functions. Returns 0
 * on success, non zero otherwise.
 */
int storage_cache_hardware_init(struct StorageDevice* device);

#endif
