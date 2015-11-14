#ifndef _LWFS_H_
#define _LWFS_H_

/**
 * Initilize a lwfs driver. Returns 0 on success, -1 otherwise.
 */
int lwfs_init(uint32_t start_sector, uint32_t end_sector, int sectsize,
                int cache_sz, char* cache, struct FSDriver* driver, void* c);

#endif
