#ifndef _DISKCACHE_H_
#define _DISKCACHE_H_

#include "devman.h"
#include "fsman.h"

/**
 * Setup an FSDriver for useing reference and dereference.
 */
int storage_cache_init(struct FSDriver* driver);

/**
 * Initilize an FSHardware driver to work with disk caching.
 */
int storage_cache_hardware_init(struct StorageDevice* driver);

#endif
