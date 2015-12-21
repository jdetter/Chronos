#ifndef _LWFS_H_
#define _LWFS_H_

/**
 * Initilize a lwfs driver. Returns 0 on success, -1 otherwise.
 */
int lwfs_init(size_t size, struct FSDriver* driver);

#endif
