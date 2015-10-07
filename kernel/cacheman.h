#ifndef _CACHEMAN_H_
#define _CACHEMAN_H_

/**
 * Initilize the cache manager during boot.
 */
void cman_init(void);

/**
 * Allocate some cache space
 */
void* cman_alloc(uint sz);

#endif
