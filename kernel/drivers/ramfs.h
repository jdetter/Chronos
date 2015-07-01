#ifndef _RAMFS_H_
#define _RAMFS_H_

void ramfs_init(void);

/**
 * Allocate and initilize a ramfs driver. Returns 0 on success.
 */
struct FSHardwareDriver* ramfs_driver_alloc(uint block_size, uint blocks);

/**
 * free a ramfs driver. Returns 0 on success.
 */
int ramfs_driver_free(struct FSHardwareDriver* driver);

#endif
