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
#include "panic.h"

#endif

#include "vm.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"
#include "drivers/storageio.h"

static int storage_cache_sync(void* ptr, sect_t sect_start, 
		struct cache* cache, void* context)
{
	struct StorageDevice* device = context;
	int sectors = PGSIZE >> device->sectshifter;
	return storageio_readsects(sect_start, sectors, ptr, PGSIZE, device);
}

static int storage_cache_populate(void* ptr, sect_t sect_start, 
		void* context)
{
	struct StorageDevice* device = context;
	int sectors = PGSIZE >> device->sectshifter;
	return storageio_readsects(sect_start, sectors, ptr, PGSIZE, device);
}

void* storage_cache_reference_global(sect_t sect, 
		struct StorageDevice* device)
{
	/* Round down to a page boundary */
	sect_t start_sect = sect & ~(device->spp - 1);
	/* Get the distance to the boundary */
	int diff = (sect - start_sect) << device->sectshifter;
	char* ptr = cache_reference(start_sect, &device->cache, device);

	/* Make sure we return a pointer to the right sector */
	return ptr + diff;
}

static void* storage_cache_reference(blk_t block_id, 
		struct FSDriver* driver)
{
	struct StorageDevice* device = driver->driver;
	/* Save the original requested block_id */
	blk_t block_id_start = block_id;
	/* Round block_id down to a page boundary */
	block_id &= ~(driver->bpp - 1);
	/* How many blocks were lost when we rounded down? */
	int diff = block_id_start - block_id;
	/* What sector is this? */
	sect_t sect_start = block_id << 
		(driver->blockshift - device->sectshifter);
	/* Get the global address of the sector */
	sect_start += driver->fs_start;

	/* reference the blocks - pointer points to start of page */
	char* ptr = cache_reference(sect_start, &device->cache, device);
	/* Adjust the pointer to point at the requested space */
	ptr += diff << driver->blockshift;
	/* Return the result*/
	return (void*)ptr;
}

void* storage_cache_addreference_global(sect_t sect,
		struct StorageDevice* device)
{
	/* Make sure this sector is on a page boundary */
	sect_t start_sect = sect & ~(device->spp - 1);
	/* Get the distance to the boundary */
	int diff = (sect - start_sect) << device->sectshifter;
	char* ptr = cache_addreference(start_sect, &device->cache, device);

	/* Make sure we return a pointer to the right sector */
	return ptr + diff;
}

static void* storage_cache_addreference(blk_t block_id, 
		struct FSDriver* driver)
{
	struct StorageDevice* device = driver->driver;
	/* Save the original requested block id*/
	blk_t block_id_start = block_id;
	/* Round down to a page boundary */
	block_id &= ~(driver->bpp - 1);
	/* Save the difference between the original and the boundary */
	int diff = block_id_start - block_id;
	/* Convert the block id into a sector id */
	sect_t sect_start = block_id << 
		(driver->blockshift - device->sectshifter);
	sect_start += driver->fs_start;

	char* ptr = cache_addreference(sect_start, &device->cache, driver);
	memset(ptr, 0, driver->blocksize); /* Clear the block */
	/* Make sure we return a pointer to the right block */
	ptr += diff << driver->blockshift;
	return ptr;
}

int storage_cache_dereference_global(void* ref, 
		struct StorageDevice* device)
{
	char* ref_c = (char*)((uintptr_t)ref & ~(PGSIZE - 1));
	return cache_dereference(ref_c, &device->cache, device);
}

static int storage_cache_dereference(void* ref, struct FSDriver* driver)
{
	char* ref_c = (char*)((uintptr_t)ref & ~(PGSIZE - 1));
	return cache_dereference(ref_c, &driver->driver->cache, driver->driver);
}

int storage_cache_init(struct FSDriver* driver)
{
	/* First, setup the driver */
	driver->reference = storage_cache_reference;
	driver->dereference = storage_cache_dereference;
	driver->addreference = storage_cache_addreference;

	return 0;
}

int storage_cache_hardware_init(struct StorageDevice* device)
{
	device->cache.populate = (void*)storage_cache_populate;
	device->cache.sync = (void*)storage_cache_sync;
	return 0;
}
