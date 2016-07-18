#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "devman.h"

#define MAX_STORAGE_DEVICES 32

/**
 * Initilize the storage device metadata structures.
 */
void storagedevice_init(void);

/**
 * Allocate a new storage device. Returns NULL if there are no more
 * storages devices to be allocated.
 */
struct StorageDevice* storagedevice_alloc(void);

/**
 * Free a storage device. This should be called when the storage
 * device is disconnected or no longer in use.
 */
void storagedevice_free(struct StorageDevice* device);

#endif
