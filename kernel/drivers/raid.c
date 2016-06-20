/**
 * Authors: Randy Amundson <randy@chronos.systems>
 *		John Detter <jdetter@chronos.systems>
 */

// #include <sys/types.h>
#include <string.h>
#include "kern/stdlib.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "panic.h"
#include "vm.h"
#include "cache.h"
#include "raid.h"

#define RAID_MAX_DISKS 32
#define RAID_MAX_CONTEXT 4
#define log2 __log2
// #define DEBUG

struct raid_context
{
	int disk_count; /* How many disks are in this raid? */
	int stride; /* How many sectors are in a stride? */
	int stride_shift; /* Quick shift for stride */
	/* Array of disk drivers (one for each disk) */
	struct FSHardwareDriver* disks[RAID_MAX_DISKS];
};

slock_t raid_context_lock;
struct raid_context raid_contexts[RAID_MAX_CONTEXT];
struct FSHardwareDriver raids[RAID_MAX_CONTEXT];

static struct FSHardwareDriver* raid_alloc(void)
{
	struct FSHardwareDriver* driver = NULL;

	slock_acquire(&raid_context_lock);

	int x;
	for(x = 0;x < RAID_MAX_CONTEXT;x++)
	{
		if(!raids[x].valid)
		{
			raids[x].valid = 1;
			driver = raids + x;
			driver->context = raid_contexts + x;
			break;
		}
	}

	slock_release(&raid_context_lock);

	return driver;
}

static void raid_free(struct FSHardwareDriver* driver)
{
	if(!driver) return;
	slock_acquire(&raid_context_lock);
	driver->valid = 0;
	slock_release(&raid_context_lock);
}

int raid_init(void)
{
	slock_init(&raid_context_lock);
	memset(raids, 0, sizeof(struct raid_context) * RAID_MAX_CONTEXT);
	memset(raids, 0, sizeof(struct FSHardwareDriver) * RAID_MAX_CONTEXT);
	return 0;
}

struct FSHardwareDriver* raid0_init(struct FSHardwareDriver** drivers, 
		int driver_count, int stride)
{
	struct FSHardwareDriver* driver = raid_alloc();
	/* did we actually get a driver? */
	if(!driver) return NULL;

	/* Create shortcut for the context */
	struct raid_context* context = driver->context;

	int min_sect = -1;
	/* Do the disks not all have the same sectsize? */
	int sects_differ = 0;
	int sect_size = -1;
	int sect_shift = -1;

        int x;
        for(x = 0;x < driver_count;x++)
        {
                context->disks[x] = drivers[x];
                if(drivers[x]->sectmax < min_sect || min_sect == -1)
                        min_sect = drivers[x]->sectmax;
                if(sect_size == -1)
                        sect_size = drivers[x]->sectsize;
                if(sect_shift == -1)
                        sect_shift = drivers[x]->sectshifter;

                if(sect_size != drivers[x]->sectsize
                                || sect_shift != drivers[x]->sectshifter)
                        sects_differ = 1;

        }

        if(sects_differ)
        {
#ifdef DEBUG
                panic("raid: disks do not have the same sector size!\n");
#endif
                raid_free(driver);
                return NULL;
        }

        context->disk_count = driver_count;
        driver->valid = 1;
        // driver->readsect = raid0_readsect;
        // if(0) driver->readsects = raid0_readsects;
        // driver->writesect = raid0_writesect;
        // if(0) driver->writesects = raid0_writesects;
        driver->sectmax = min_sect;
        driver->sectshifter = sect_shift;
        driver->sectsize = sect_size;

	/* Set the stride equal to the page size */
	context->stride = stride;
	context->stride_shift = log2(stride);

        /* Setup the cache manager for this driver */
        memset(&driver->cache, 0, sizeof(struct cache));

        return driver;
}

int raid0_setup_cache(struct FSDriver* driver, struct FSHardwareDriver* raid)
{
        if(!driver || !raid) return -1;
        // driver->reference = raid0_cache_reference;
        // driver->addreference = raid0_cache_addreference;
        // driver->dereference = raid0_cache_dereference;
        return 0;
}

/** VVV TODO: Start implementing here VVV*/

int raid0_readsect(void* dst, sect_t sect, 
		struct FSHardwareDriver* driver)
{
	struct raid_context* context = driver->context;

	int disk = sect % context->disk_count;
	sect_t physical = sect / context-> disk_count;

	if(context->disks[disk]->readsect(dst, physical, 
				context->disks[disk]) != 0)
		return -1;
	else return 0;
}

int raid0_readsects(void* dst, sect_t sectstart, int sectcount, 
		struct FSHardwareDriver* driver)
{
	int current = 0;//postion from sectstart

	while(current < sectcount){//how close to the end
		if(raid0_readsect(dst, sectstart + current, driver) == -1)
		{
			return -1;
		}
		current++;
	}
	return 0;
}

int raid0_writesect(void* src, sect_t sect, 
		struct FSHardwareDriver* driver)
{
	struct raid_context* context = driver->context;

	int disk = sect % context->disk_count;
	sect_t physical = sect / context->disk_count;

	if(context->disks[disk]->writesect(src, physical, 
				context->disks[disk]) != 0)
		return -1;
	else return 0;
}

int raid0_writesects(void* src, sect_t sectstart, int sectcount, 
		struct FSHardwareDriver* driver) 
{

	int current = 0;//postion from sectstart

	while(current < sectcount){//how close to the end
		if(raid0_writesect(src, sectstart + current, driver) == -1)
		{
			return -1;
		}
		current++;
	}
	return 0;

}

void* raid0_cache_reference(int id, struct FSDriver* driver)
{
	struct FSHardwareDriver* raid = driver->driver;
	struct raid_context* context = raid->context;

	blk_t local_id = (blk_t)id;
        /* Convert this from the drivers view to the global view */
        int block_to_sect = driver->blockshift - driver->driver->sectshifter;
        /* Which start sector is this on disk? */
        sect_t gbl_sect = (local_id << block_to_sect) + driver->start;

	/* Which disk is this on? */
	int disk_num = -1;
	int local_sect = -1;
	if(context->stride_shift)
	{
		disk_num = (gbl_sect >> context->stride_shift) 
			% context->disk_count;
		int local_stride = (gbl_sect >> context->stride_shift)
			/ context->disk_count;
		local_sect = (local_stride << context->stride_shift)
			+ (gbl_sect & (context->stride_shift - 1));
	} else {
		disk_num = (gbl_sect / context->stride) 
			% context->disk_count;
                int local_stride = (gbl_sect / context->stride) 
			/ context->disk_count;
                local_sect = (local_stride * context->stride)
                        + gbl_sect % context->stride;
	}

#ifdef DEBUG
	if(disk < 0 || disk >= context->disk_count || local_sect < 0)
		panic("raid0: invalid sector!\n");
#endif

	struct FSHardwareDriver* disk = context->disks[disk_num];

        /* Save the original requested block_id */
        sect_t sect_start = local_sect;
        /* Round global sect down to a page boundary */
        local_sect &= ~(disk->spp - 1);
        /* How many blocks were lost when we rounded down? */
        int diff = (sect_start - gbl_sect) >> block_to_sect;

        /* reference the blocks - pointer points to start of page */
        char* bp = cache_reference(local_sect,
                        &driver->driver->cache, driver);

        /* Adjust the pointer to point at the requested space */
        bp += diff << driver->blockshift;

#ifdef DEBUG
	cprintf("raid0: reference for global %d\n", global_id);
	cprintf("raid0: this is local %d on disk %d\n", block_id_start, 
		disk_num);
	cprintf("raid0: block_start: %d actual: %d diff: %d\n",
			block_id_start, local_id, diff);
	cprintf("raid0: returned pointer: 0x%x\n", bp);
#endif

	/* Return the result*/
	return (void*)bp;
}

void* raid0_cache_addreference(int id,
		struct FSDriver* driver)
{
	blk_t global_id = (blk_t)id;
        struct FSHardwareDriver* raid = driver->driver;
        struct raid_context* context = raid->context;
        int disk_num = global_id % context->disk_count;
        int local_id = (global_id - disk_num) / context->disk_count;
        struct FSHardwareDriver* disk = context->disks[disk_num];

	uint block_id_start = local_id;
	local_id &= ~(driver->bpp - 1);
	uint diff = block_id_start - local_id;
	char* block_ptr = cache_addreference(local_id,
			&disk->cache, driver);
	memset(block_ptr, 0, driver->blocksize); /* Clear the block */
	block_ptr += diff << driver->blockshift;
#ifdef DEBUG
	cprintf("raid0: reference for global %d\n", global_id);
	cprintf("raid0: this is local %d on disk %d\n", local_id, disk_num);
	cprintf("raid0: block_start: %d actual: %d diff: %d\n",
			block_id_start, local_id, diff);
	cprintf("raid0: returned pointer: 0x%x\n", block_ptr);
#endif
	return block_ptr;
}

int raid0_cache_dereference(void* ref, struct FSDriver* driver)
{
	if(!ref) return 0;
        struct FSHardwareDriver* raid = driver->driver;
        struct raid_context* context = raid->context;
        struct FSHardwareDriver* disk = NULL;

        /* Which disk cache does this ref belong to? */
        int x;
        for(x = 0;x < context->disk_count;x++)
        {
                disk = context->disks[x];
                if(!context->disks[x]->cache.slab_shift)
                        panic("Bad raid cache!\n");
                int slabs_sz = disk->cache.entry_count
                        << disk->cache.slab_shift;
                if((char*)ref >= disk->cache.slabs &&
                                (char*)ref < disk->cache.slabs + slabs_sz)
                        break;
        }
        
	/* ref must be rounded to a page boundary */
	char* ref_c = ref;
        ref_c = (char*)((uint)ref_c & ~(PGSIZE - 1));

#ifdef DEBUG
	cprintf("raid0: dereferencing 0x%x\n", ref);
	cprintf("raid0: disk for this pointer: 0x%x\n", x);
#endif

        /* Invalid pointer */
        if(x >= context->disk_count) return -1;

        return cache_dereference(ref_c, &disk->cache, disk);
}

struct FSHardwareDriver* raid1_init(struct FSHardwareDriver** drivers, 
		int driver_count, int stride)
{
	struct FSHardwareDriver* driver = raid_alloc();
	/* did we actually get a driver? */
	if(!driver) return NULL;

	/* Create shortcut for the context */
	struct raid_context* context = driver->context;

	int min_sect = -1;
	/* Do the disks not all have the same sectsize? */
	int sects_differ = 0;
	int sect_size = -1;
	int sect_shift = -1;

	int x;
	for(x = 0;x < driver_count;x++)
	{
		context->disks[x] = drivers[x];
		if(drivers[x]->sectmax < min_sect || min_sect == -1)
			min_sect = drivers[x]->sectmax;
		if(sect_size == -1)
			sect_size = drivers[x]->sectsize;
		if(sect_shift == -1)
			sect_shift = drivers[x]->sectshifter;

		if(sect_size != drivers[x]->sectsize
				|| sect_shift != drivers[x]->sectshifter)
			sects_differ = 1;
			
	}

	if(sects_differ)
	{
#ifdef DEBUG
		panic("raid: disks do not have the same sector size!\n");
#endif
		raid_free(driver);
		return NULL;
	}

	context->disk_count = driver_count;
	context->stride = stride;
	driver->valid = 1;
	// driver->readsect = raid1_readsect;
	// if(0) driver->readsects = raid1_readsects;
	// driver->writesect = raid1_writesect;
	// if(0) driver->writesects = raid1_writesects;
	driver->sectmax = min_sect;
	driver->sectshifter = sect_shift;
	driver->sectsize = sect_size;

	/* Setup the cache manager for this driver */
	memset(&driver->cache, 0, sizeof(struct cache));

	return driver;
}

int raid1_setup_cache(struct FSDriver* driver, struct FSHardwareDriver* raid)
{
	if(!driver || !raid) return -1;
	// driver->reference = raid1_cache_reference;
	// driver->addreference = raid1_cache_addreference;
	// driver->dereference = raid1_cache_dereference;
	return 0;
}

int raid1_readsect(void* dst, sect_t sect,
		struct FSHardwareDriver* driver)
{
	struct raid_context* context = driver->context;

	int disk = 0;
	if(context->disks[disk]->readsect(dst, sect,
				context->disks[disk]) != 0)
	{
		return -1;
	}

	return 0;
}

int raid1_readsects(void* dst, sect_t sectstart, int sectcount,
		struct FSHardwareDriver* driver)
{

	int current = 0;//postion from sectstart

	while(current < sectcount){//how close to the end
		if(raid1_readsect(dst, sectstart + current, driver) == -1)
		{
			return -1;
		}
		current++;
	}
	return 0;  
}

int raid1_writesect(void* src, sect_t sect, 
		struct FSHardwareDriver* driver)
{

	struct raid_context* context = driver->context;

	int x = 0; 
	for(x = 0; x < context->disk_count;x++){
		if(context->disks[x]->writesect(src, sect,
					context->disks[x]) != 0)
		{
			return -1;
		}
	}
	return 0;
}

int raid1_writesects(void* src, sect_t sectstart, int sectcount,
		struct FSHardwareDriver* driver)
{
	int current = 0;//postion from sectstart

	while(current < sectcount){//how close to the end
		if(raid1_writesect(src, sectstart + current, driver) == -1)
		{
			return -1;
		}
		current++;
	}
	return 0; 	
}

void* raid1_cache_reference(int id, struct FSDriver* driver)
{
	blk_t block_id = (blk_t)id;
	struct FSHardwareDriver* raid = driver->driver;
	struct raid_context* context = raid->context;
	struct FSHardwareDriver* disk = context->disks[0];
	/* Just read from disk 1 */
	/* Save the original requested block_id */
	uint block_id_start = block_id;
	/* Round block_id down to a page boundary */
	block_id &= ~(driver->bpp - 1);
	/* How many blocks were lost when we rounded down? */
	uint diff = block_id_start - block_id;

	/* reference the blocks - pointer points to start of page */
	char* bp = cache_reference(block_id, &disk->cache, driver);

	/* Adjust the pointer to point at the requested space */
	bp += diff << driver->blockshift;

	/* Return the result*/
	return (void*)bp;
}

void* raid1_cache_addreference(int id, struct FSDriver* driver)
{
	blk_t block_id = (blk_t)id;
	struct FSHardwareDriver* raid = driver->driver;
	struct raid_context* context = raid->context;
	struct FSHardwareDriver* disk = context->disks[0];

	uint block_id_start = block_id;
	block_id &= ~(driver->bpp - 1);
	uint diff = block_id_start - block_id;
	char* block_ptr = cache_addreference(block_id,
			&disk->cache, driver);
	memset(block_ptr, 0, driver->blocksize); /* Clear the block */
	block_ptr += diff << driver->blockshift;
	return block_ptr;
}

int raid1_cache_dereference(void* ref, struct FSDriver* driver)
{
	if(!ref) return 0;
	struct FSHardwareDriver* raid = driver->driver;
	struct raid_context* context = raid->context;
	struct FSHardwareDriver* disk = context->disks[0];

	/* ref must be rounded to a page boundary */
	char* ref_c = ref;
	ref_c = (char*)((uint)ref_c & ~(PGSIZE - 1));

	return cache_dereference(ref_c, &disk->cache, disk);
}

struct FSHardwareDriver* raid5_init(struct FSHardwareDriver** drivers,
                int driver_count, int stride)
{
	struct FSHardwareDriver* driver = raid_alloc();
	/* did we actually get a driver? */
	if(!driver) return NULL;

	/* Create shortcut for the context */
	struct raid_context* context = driver->context;

	int min_sect = -1;
	/* Do the disks not all have the same sectsize? */
	int sects_differ = 0;
	int sect_size = -1;
	int sect_shift = -1;

	int x;
	for(x = 0;x < driver_count;x++)
	{
		context->disks[x] = drivers[x];
		if(drivers[x]->sectmax < min_sect || min_sect == -1)
			min_sect = drivers[x]->sectmax;
		if(sect_size == -1)
			sect_size = drivers[x]->sectsize;
		if(sect_shift == -1)
			sect_shift = drivers[x]->sectshifter;

		if(sect_size != drivers[x]->sectsize
				|| sect_shift != drivers[x]->sectshifter)
			sects_differ = 1;
			
	}

	if(sects_differ)
	{
#ifdef DEBUG
		panic("raid: disks do not have the same sector size!\n");
#endif
		raid_free(driver);
		return NULL;
	}

	context->disk_count = driver_count;
	driver->valid = 1;
	// driver->readsect = raid5_readsect;
	// if(0) driver->readsects = raid5_readsects;
	// driver->writesect = raid5_writesect;
	// if(0) driver->writesects = raid5_writesects;
	driver->sectmax = min_sect;
	driver->sectshifter = sect_shift;
	driver->sectsize = sect_size;

	context->stride = stride;
	context->stride_shift = log2(stride);

	/* Setup the cache manager for this driver */
	memset(&driver->cache, 0, sizeof(struct cache));

	return driver;
}

int raid5_setup_cache(struct FSDriver* driver, struct FSHardwareDriver* raid)
{
        if(!driver || !raid) return -1;
        // driver->reference = raid5_cache_reference;
        // driver->addreference = raid5_cache_addreference;
        // driver->dereference = raid5_cache_dereference;
        return 0;
}

int raid5_readsect(void* dst, sect_t sect,
                struct FSHardwareDriver* driver)
{
	/* TODO: implementation needed */
	return -1;
}

int raid5_readsects(void* dst, sect_t sectstart, int sectcount,
                struct FSHardwareDriver* driver)
{
	/* TODO: implementation needed */
	return -1;
}

int raid5_writesect(void* src, sect_t sect,
                struct FSHardwareDriver* driver)
{
	/* TODO: implementation needed */
	return -1;
}

int raid5_writesects(void* src, sect_t sectstart, int sectcount,
                struct FSHardwareDriver* driver)
{
	/* TODO: implementation needed */
	return -1;
}

void* raid5_cache_reference(int block_id, struct FSDriver* driver)
{
	return NULL;
}

void* raid5_cache_addreference(int block_id,
                struct FSDriver* driver)
{
	return NULL;
}

int raid5_cache_dereference(void* ref, struct FSDriver* driver)
{
	return 0;
}
