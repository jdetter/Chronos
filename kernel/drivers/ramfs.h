#ifndef _RAMFS_H_
#define _RAMFS_H_

void ramfs_init(void);

struct ramfs_context;

/**
 * Enter the ramfs address space. It is safe to call this meathod more
 * than once.
 */
void ramfs_enter(struct ramfs_context* c);

/**
 * Exit the ramfs address space.
 */
void ramfs_exit(struct ramfs_context* c);

/**
 * Allocate and initilize a ramfs driver. Returns 0 on success.
 */
struct FSHardwareDriver* ramfs_driver_alloc(uint block_size, uint blocks);

/**
 * free a ramfs driver. Returns 0 on success.
 */
int ramfs_driver_free(struct FSHardwareDriver* driver);

#endif
