/**
 * Authors: John Detter <jdetter@chronos.systems>
 * 		Randy Amundson <randy@chronos.systems>
 */

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

static int disk_cache_sync(void* obj, int block_id, 
		struct cache* cache, void* context)
{
	struct FSDriver* driver = context;
	/* Write the block */
	if(driver->writeblock(obj, block_id, driver) 
			!= driver->blocksize)
		return -1;
	return 0;
}

static int disk_cache_populate(void* block, int block_id, void* context)
{
	struct FSDriver* driver = context;
	return (driver->readblock(block, block_id, driver)) 
		!= driver->blocksize;
}

static void* disk_cache_reference(uint block_id, struct FSDriver* driver)
{
	return cache_reference(block_id, &driver->driver->cache, driver);
}

static void* disk_cache_addreference(uint block_id, 
		struct FSDriver* driver)
{
	void* block_ptr = cache_addreference(block_id, 
		&driver->driver->cache, driver);
	memset(block_ptr, 0, driver->blocksize); /* Clear the block */
	return block_ptr;
}

static int disk_cache_dereference(void* ref, struct FSDriver* driver)
{
	return cache_dereference(ref, &driver->driver->cache);
}

int disk_cache_hardware_init(struct FSHardwareDriver* driver, 
		void* cache, uint sz)
{
	driver->cache.populate = disk_cache_populate;
	driver->cache.sync = disk_cache_sync;
	return 0;
}

int disk_cache_init(struct FSDriver* driver)
{
	/* First, setup the driver */
	driver->reference = disk_cache_reference;
	driver->dereference = disk_cache_dereference;
	driver->addreference = disk_cache_addreference;

	return 0;
}
