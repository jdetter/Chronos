#ifndef _CACHEMAN_H_
#define _CACHEMAN_H_

/**
 * Initilize the cache manager during boot.
 */
void cman_init(void);

/**
 * Allocate some cache space
 */
void* cman_alloc(size_t sz);

/**
 * Free the cache space.
 */
void cman_free(void* ptr, size_t size);

#endif
