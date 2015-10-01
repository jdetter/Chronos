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

#include "stdlib.h"

#endif

#include "types.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"

static int disk_cache_sync(void* obj, int id, struct cache* cache)
{
	struct FSDriver* driver = cache->context;
	/* Write the block */
	if(driver->writeblock(obj, id, driver) != driver->blocksize)
		return -1;
	return 0;
}

static int disk_cache_populate(void* block, int id, void* context)
{
	struct FSDriver* driver = context;
	return (driver->readblock(block, id, driver)) != driver->blocksize;
}

static void* disk_cache_reference(uint block, struct FSDriver* driver)
{
	return cache_reference(block, &driver->cache);
}

static void* disk_cache_addreference(uint block, 
		struct FSHardwareDriver* driver)
{
	void* block_ptr = cache_addreference(block, &driver->cache);
	memset(block_ptr, 0, driver->blocksize); /* Clear the block */
	return block_ptr;
}

static int disk_cache_dereference(void* ref, struct FSHardwareDriver* driver)
{
	return cache_dereference(ref, &driver->cache);
}

int disk_cache_init(struct FSHardwareDriver* driver)
{
	/* First, setup the driver */
	driver->reference = disk_cache_reference;
	driver->dereference = disk_cache_dereference;
	driver->addreference = disk_cache_addreference;
	/* Setup the cache with our sync function */
	driver->cache.sync = disk_cache_sync;
	driver->cache.populate = disk_cache_populate;

	return 0;
}
