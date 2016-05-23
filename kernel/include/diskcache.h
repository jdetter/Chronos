#ifndef _DISKCACHE_H_
#define _DISKCACHE_H_

/**
 * Setup an FSDriver for useing reference and dereference.
 */
int disk_cache_init(struct FSDriver* driver);

/**
 * Initilize an FSHardware driver to work with disk caching.
 */
int disk_cache_hardware_init(struct FSHardwareDriver* driver);

#endif
