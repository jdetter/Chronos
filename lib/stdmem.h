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
 * minit should setup the global variables needed for this allocator to 
 * function. It should call mmap and request an amount of memory. This memory 
 * amount should be defined using #define MEM_AMT so that it can easily be 
 * changed for tuning.
 */
void minit(void);

#endif
