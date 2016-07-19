/**
 * Authors: Randy Amundson <randy@chronos.systems>
 *		John Detter <jdetter@chronos.systems>
 */

// #include <sys/types.h>
#include <string.h>
#include "kstdlib.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "panic.h"
#include "vm.h"
#include "cache.h"
#include "raid.h"
#include "storage.h"

#define RAID_MAX_DISKS 32
#define RAID_MAX_CONTEXT 4
#define log2 __log2
// #define DEBUG

struct raid_context
{
	int valid; /* Whether or not this raid context is in use */
	int disk_count; /* How many disks are in this raid? */
	int stride; /* How many sectors are in a stride? */
	int stride_shift; /* Quick shift for stride */
	/* Array of disk drivers (one for each disk) */
	struct StorageDevice* disks[RAID_MAX_DISKS];
};

slock_t raid_context_lock;
struct raid_context raid_contexts[RAID_MAX_CONTEXT];

static struct StorageDevice* raid_alloc(void)
{
	struct StorageDevice* driver = NULL;

	slock_acquire(&raid_context_lock);
	int x;
	for(x = 0;x < RAID_MAX_CONTEXT;x++)
	{
		if(!raid_contexts[x].valid)
		{
			driver = storagedevice_alloc();
			if(!driver)
				break;
			driver->context = raid_contexts + x;
			memset(driver->context, 0, sizeof(struct raid_context));
			raid_contexts[x].valid = 1;
			break;
		}
	}
	slock_release(&raid_context_lock);

	return driver;
}

static void raid_free(struct StorageDevice* driver)
{
	slock_acquire(&raid_context_lock);

	/* Free the context */
	struct raid_context* raid = driver->context;
	raid->valid = 0;
	/* Free the storage driver */
	storagedevice_free(driver);

	slock_release(&raid_context_lock);
}

void raid_init(void)
{
	slock_init(&raid_context_lock);
	memset(raid_contexts, 0, sizeof(struct raid_context) * RAID_MAX_CONTEXT);
}

static int raid0_cache_sync(void* ptr, sect_t sect_start, struct cache* cache, 
		void* context);
static int raid0_cache_populate(void* ptr, sect_t sect_start, void* context);

struct StorageDevice* raid0_init(struct StorageDevice** devices, 
		int driver_count, int stride)
{
	struct StorageDevice* driver = raid_alloc();
	/* did we actually get a driver? */
	if(!driver) return NULL;

	/* Create shortcut for the context */
	struct raid_context* context = driver->context;

	int sectors = -1;
	/* Do the disks not all have the same sectsize? */
	int sects_differ = 0;
	int sect_size = -1;
	int sect_shift = -1;

	int x;
	for(x = 0;x < driver_count;x++)
	{
		context->disks[x] = devices[x];
		if(devices[x]->sectors < sectors || sectors == -1)
			sectors = devices[x]->sectors;
		if(sect_size == -1)
			sect_size = devices[x]->sectsize;
		if(sect_shift == -1)
			sect_shift = devices[x]->sectshifter;
		if(sect_size != devices[x]->sectsize
				|| sect_shift != devices[x]->sectshifter)
			sects_differ = 1;
	}

	if(sects_differ)
	{
#ifdef DEBUG
		cprintf("raid: disks do not have the same sector size!\n");
#endif
		raid_free(driver);
		return NULL;
	}

	context->disk_count = driver_count;
	// driver->readsect = raid0_readsect;
	// driver->readsects = raid0_readsects;
	// driver->writesect = raid0_writesect;
	// driver->writesects = raid0_writesects;
	driver->sectors = sectors;
	driver->sectshifter = sect_shift;
	driver->sectsize = sect_size;

	driver->cache.sync = (void*)raid0_cache_sync;
	driver->cache.populate = (void*)raid0_cache_populate;

	/* Set the stride equal to the page size */
	context->stride = stride;
	context->stride_shift = log2(stride);

	/* Setup the cache manager for this driver */
	memset(&driver->cache, 0, sizeof(struct cache));

	return driver;
}

/** VVV TODO: Start implementing here VVV*/

static int raid0_cache_sync(void* ptr, sect_t sect_start, struct cache* cache, 
		void* context)
{
	struct StorageDevice* raid_device = context;
	struct raid_context* raid = raid_device->context;
	/* How many sectors fit on a page? */
	int sectors = raid_device->spp;

	return 0;
}

static int raid0_cache_populate(void* ptr, sect_t sect_start, void* context)
{
	struct StorageDevice* raid_device = context;
	struct raid_context* raid = raid_device->context;
	/* How many sectors fit on a page? */
	int sectors = raid_device->spp;

	return 0;
}

static int raid1_cache_sync(void* ptr, sect_t sect_start, 
		struct cache* cache, void* context);
static int raid1_cache_populate(void* ptr, sect_t sect_start, void* context);

struct StorageDevice* raid1_init(struct StorageDevice** devices, 
		int driver_count, int stride)
{
	struct StorageDevice* driver = raid_alloc();
	/* did we actually get a driver? */
	if(!driver) return NULL;

	/* Create shortcut for the context */
	struct raid_context* context = driver->context;

	int sectors = -1;
	/* Do the disks not all have the same sectsize? */
	int sects_differ = 0;
	int sect_size = -1;
	int sect_shift = -1;

	int x;
	for(x = 0;x < driver_count;x++)
	{
		context->disks[x] = devices[x];
		if(devices[x]->sectors < sectors || sectors == -1)
			sectors = devices[x]->sectors;
		if(sect_size == -1)
			sect_size = devices[x]->sectsize;
		if(sect_shift == -1)
			sect_shift = devices[x]->sectshifter;
		if(sect_size != devices[x]->sectsize
				|| sect_shift != devices[x]->sectshifter)
			sects_differ = 1;
	}

	if(sects_differ)
	{
#ifdef DEBUG
		cprintf("raid: disks do not have the same sector size!\n");
#endif
		raid_free(driver);
		return NULL;
	}

	context->disk_count = driver_count;
	// driver->readsect = raid0_readsect;
	// driver->readsects = raid0_readsects;
	// driver->writesect = raid0_writesect;
	// driver->writesects = raid0_writesects;
	driver->cache.sync = (void*)raid1_cache_sync;
	driver->cache.populate = (void*)raid1_cache_populate;
	driver->sectors = sectors;
	driver->sectshifter = sect_shift;
	driver->sectsize = sect_size;

	/* Set the stride equal to the page size */
	context->stride = stride;
	context->stride_shift = log2(stride);

	/* Setup the cache manager for this driver */
	memset(&driver->cache, 0, sizeof(struct cache));

	return driver;
}

static int raid1_cache_sync(void* src, sect_t sector, struct cache* cache,
		void* context)
{
	struct StorageDevice* raid_device = context;
	struct raid_context* raid = raid_device->context;
	/* How many sectors fit on a page? */
	int sectors = raid_device->spp;

	int x;
	for(x = 0;x < raid->disk_count;x++)
	{
		if(raid->disks[x]->writesects(sector, sectors, src, PGSIZE, raid->disks[x]))
			return -1;
	}

	return 0;
}

static int raid1_cache_populate(void* dst, sect_t sector, void* context)
{
	struct StorageDevice* raid_device = context;
	struct raid_context* raid = raid_device->context;
	/* How many sectors fit on a page? */
	int sectors = raid_device->spp;

	if(raid->disks[0]->readsects(sector, sectors, dst, PGSIZE, raid->disks[0]))
		return -1;

	return 0;
}

