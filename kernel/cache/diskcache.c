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

#include <stdlib.h>
#include <string.h>
#include "kern/stdlib.h"

#endif

#include "kern/types.h"
#include "x86.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"

static int disk_cache_sync(void* obj, int block_id, 
		struct cache* cache, void* context)
{
	struct FSDriver* driver = context;
	return driver->writeblocks(obj, block_id, driver->bpp, driver);
}

static int disk_cache_populate(void* blocks, int block_id, void* context)
{
	struct FSDriver* driver = context;
	return driver->readblocks(blocks, block_id, driver->bpp,  driver);
}

static void* disk_cache_reference(uint block_id, struct FSDriver* driver)
{
	/* Save the original requested block_id */
	uint block_id_start = block_id;
	/* Round block_id down to a page boundary */
	block_id &= ~(driver->bpp - 1);
	/* How many blocks were lost when we rounded down? */
	uint diff = block_id_start - block_id;

	/* reference the blocks - pointer points to start of page */
	char* bp = cache_reference(block_id, &driver->driver->cache, driver);

	/* Adjust the pointer to point at the requested space */
	bp += diff << driver->blockshift;

	/* Return the result*/
	return (void*)bp;
}

static void* disk_cache_addreference(uint block_id, 
		struct FSDriver* driver)
{
	uint block_id_start = block_id;
	block_id &= ~(driver->bpp - 1);
	uint diff = block_id_start - block_id;
	char* block_ptr = cache_addreference(block_id, 
		&driver->driver->cache, driver);
	memset(block_ptr, 0, driver->blocksize); /* Clear the block */
	block_ptr += diff << driver->blockshift;
	return block_ptr;
}

static int disk_cache_dereference(void* ref, struct FSDriver* driver)
{
	/* ref must be rounded to a page boundary */
	char* ref_c = ref;
	ref_c = (char*)((uint)ref_c & ~(PGSIZE - 1));

	return cache_dereference(ref_c, &driver->driver->cache, driver);
}

int disk_cache_hardware_init(struct FSHardwareDriver* driver)
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
