#ifndef _STDMEM_H_
#define _STDMEM_H_

/**
 * Allocate sz bytes on the heap. You should return a pointer to the allocated 
 * region. For performance reasons, the allocated block should be 16 byte 
 * aligned.
 */
void* malloc(uint sz);

/**
 * Free the block that was eariler allocated by malloc. The free list should 
 * be coalesced to try to make the largest free blocks possible. Returns
 * whether or not the block was freed.
 */
int mfree(void* ptr);

/**
 * If mem_map is 1, then minit should ignore start_addr and end_addr and
 * map a page into memory and use that as it's allocator space. If mem_map
 * is 0, the start_addr and end_addr will be used as the space for the
 * memory allocator.
 */
void minit(uint start_addr, uint end_addr, uint mem_map);

#endif
