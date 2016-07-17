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

#define cprintf printf
/* need the log2 algorithm */
int log2_linux(int value); /* defined in ext2.c*/

#define log2(val) log2_linux(val)

#else

#include <stdlib.h>
#include <string.h>
#include "kstdlib.h"

#endif

#include "vm.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"

static int storage_cache_sync(void* obj, int block_id, 
		struct cache* cache, void* context)
{
	struct FSDriver* driver = context;
	return driver->writeblocks(obj, block_id, driver->bpp, driver);
}

static int storage_cache_populate(void* blocks, int block_id, void* context)
{
	struct FSDriver* driver = context;
	return driver->readblocks(blocks, block_id, driver->bpp,  driver);
}

static void* storage_cache_reference(blk_t block_id, struct FSDriver* driver)
{
	/* Save the original requested block_id */
	blk_t block_id_start = block_id;
	/* Round block_id down to a page boundary */
	block_id &= ~(driver->bpp - 1);
	/* How many blocks were lost when we rounded down? */
	int diff = block_id_start - block_id;

	/* reference the blocks - pointer points to start of page */
	char* bp = cache_reference(block_id, &driver->driver->cache, driver);

	/* Adjust the pointer to point at the requested space */
	bp += diff << driver->blockshift;

	/* Return the result*/
	return (void*)bp;
}

static void* storage_cache_addreference(blk_t block_id, 
		struct FSDriver* driver)
{
	blk_t block_id_start = block_id;
	block_id &= ~(driver->bpp - 1);
	int diff = block_id_start - block_id;
	char* block_ptr = cache_addreference(block_id, 
		&driver->driver->cache, driver);
	memset(block_ptr, 0, driver->blocksize); /* Clear the block */
	block_ptr += diff << driver->blockshift;
	return block_ptr;
}

static int storage_cache_dereference(void* ref, struct FSDriver* driver)
{
	/* ref must be rounded to a page boundary */
	char* ref_c = ref;
	ref_c = (char*)((uintptr_t)ref_c & ~(PGSIZE - 1));

	return cache_dereference(ref_c, &driver->driver->cache, driver);
}

int storage_cache_hardware_init(struct StorageDevice* driver)
{
	driver->cache.populate = storage_cache_populate;
	driver->cache.sync = storage_cache_sync;
	return 0;
}

int storage_cache_init(struct FSDriver* driver)
{
	/* First, setup the driver */
	driver->reference = storage_cache_reference;
	driver->dereference = storage_cache_dereference;
	driver->addreference = storage_cache_addreference;

	return 0;
}
