#ifdef __LINUX__
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>

#define uint_64 uint64_t
#define uint_32 uint32_t
#define uint_16 uint16_t
#define uint_8  uint8_t
#define cprintf printf
/* need the log2 algorithm */
int log2_linux(uint value); /* defined in ext2.c*/

#define log2(val) log2_linux(val)

#else

#endif

#include "types.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"

#define DISK_CACHE_DEBUG

void* disk_cache_reference(uint block, struct FSHardwareDriver* driver)
{
	slock_acquire(&driver->cache_lock);
	void* block_ptr = driver->cache.search(block, &driver->cache);
#ifdef DISK_CACHE_DEBUG
	if(block_ptr)
		cprintf("cache: Block %d was found in cache.\n", block);
	else cprintf("cache: Block %d was not found in cache!\n", block);
#endif
	
	if(!block_ptr)
		block_ptr = driver->cache.alloc(block, 1, &driver->cache);

	slock_release(&driver->cache_lock);

#ifdef DISK_CACHE_DEBUG
	if(!block_ptr)
		cprintf("cache: out of space!\n");
#endif

	return block_ptr;
}

int disk_cache_dereference(void* ref, struct FSHardwareDriver* driver)
{
	return 0;
}
