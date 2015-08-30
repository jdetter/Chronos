#ifdef __LINUX__
#include "types.h"
#include "stdlock.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define uint_64 uint64_t
#define uint_32 uint32_t
#define uint_16 uint16_t
#define uint_8  uint8_t
#define cprintf printf
/* need the log2 algorithm */
int log2_linux(uint value); /* defined in ext2.c*/

#define log2(val) log2_linux(val)
#else
#include "types.h"
#include "stdlock.h"
#include "stdlib.h"
#endif

#include "cacheman.h"

// #define CACHE_DEBUG

struct cache_entry
{
	int id; /* A unique identifier that can be used for search.*/
	int references;	/* How many hard pointers are there? */
	void* slab; /* A pointer to the data that goes with this entry. */
	int unused; /* Keep 2 byte alignment */
};

static int cache_default_check(void* obj, int id, struct cache* cache)
{
	struct cache_entry* entry = cache->entries;

	if(cache->slab_shift)
	{
		/* We can use shifting (fast) */
		entry += (((uint)obj - (uint)cache->slabs) 
			>> cache->slab_shift);
	} else {
		/* We have to use division (slow) */
		entry += (((uint)obj - (uint)cache->slabs) 
			/ cache->slab_sz);
	}

	/* Match */
	if(entry->id == id) return 0;

	/* No match */
	return -1;
}

int cache_dump(struct cache* cache)
{
	int x;
	for(x = 0;x < cache->entry_count;x++)
	{
		if(cache->entries[x].id)
		{
			cprintf("%d has %d references.\n",
				cache->entries[x].id,
				cache->entries[x].references);
		}
	}

	return 0;
}

static void* cache_search_nolock(int id, struct cache* cache);
static int cache_dereference_nolock(void* ptr, struct cache* cache);

int cache_init(void* cache_area, uint sz, uint data_sz, 
		void* context, char* name, struct cache* cache)
{
	memset(cache, 0, sizeof(struct cache));
	uint entries = sz / (sizeof(struct cache_entry) + data_sz);
	if(entries < 1) return -1;

#ifdef CACHE_DEBUG
	cprintf("%s cache: there are %d entries available in this cache.\n", 
		name, entries);
#endif

	cache->entry_shift = log2(sizeof(struct cache_entry));
	if(cache->entry_shift < 0)
	{
#ifdef CACHE_DEBUG
		cprintf("%s cache: entry size is not a multiple of 2.\n", 
			name);
#endif
		return -1;
	}
	cache->slab_sz = data_sz;
	cache->context = context;
	cache->entry_count = entries;
	cache->slabs = cache_area;
	cache->clock = 0;
	cache->entries = (void*)(cache->slabs + sz) 
		- (entries << cache->entry_shift);
	cache->last_entry = (uint)(cache->entries + (entries - 1));
	strncpy(cache->name, name, 64);
	slock_init(&cache->lock);
	memset(cache_area, 0, sz); /* Clear to 0 */

	/* Setup slab pointers */
	int x;
	for(x = 0;x < entries;x++)
		cache->entries[x].slab = cache->slabs + (data_sz * x);

	/* Try to assign a shift value */
	cache->slab_shift = log2(data_sz);
	/* Check to see if it worked */
	if(cache->slab_shift < 0) 
	{
#ifdef CACHE_DEBUG
		cprintf("%s cache: slab value is not a multiple of 2.\n", 
			name);
#endif
		cache->slab_shift = 0;
	}

	/* Default search function */
	cache->check = cache_default_check;

	return 0;
}

static void* cache_alloc(int id, struct cache* cache)
{
        void* result = NULL;

        int x;
        for(x = 0;x < cache->entry_count;x++)
        {
                if(!cache->entries[x].references)
                {
			result = cache->entries[x].slab;
			cache->entries[x].id = id;
			cache->entries[x].references = 1;
			break;
		}
        }

#ifdef CACHE_DEBUG
        if(result) cprintf("%s cache: new object allocated: %d\n", 
			cache->name, id);
        else cprintf("%s cache: not enough room in cache!\n", cache->name);
#endif

        return result;
}

static int cache_force_free(void* ptr, struct cache* cache)
{
        struct cache_entry* entry = cache->entries;
        uint val = (uint)ptr - (uint)cache->slabs;
        /* If shift is available then use it (fast) */
        if(cache->slab_shift)
                entry += (uint)(val >> cache->slab_shift);
        else {
                /* The division instruction is super slow. */
                entry += (val / cache->slab_sz);
        }
	if((uint)entry > cache->last_entry)
		return -1;

	entry->references = 0;
	entry->id = 0;

	return 0;
}

static int cache_dereference_nolock(void* ptr, struct cache* cache)
{
	if(!ptr) return -1;
        int result = 0;
        struct cache_entry* entry = cache->entries;
        uint val = (uint)ptr - (uint)cache->slabs;
        /* If shift is available then use it (fast) */
        if(cache->slab_shift)
                entry += (uint)(val >> cache->slab_shift);
        else {
                /* The division instruction is super slow. */
                entry += (val / cache->slab_sz);
        }
	if((uint)entry >= cache->last_entry)
		return -1;
        entry->references--;
        if(entry->references <= 0)
        {
                entry->references = 0;
#ifdef CACHE_DEBUG
                cprintf("%s cache: object %d fully dereferenced.\n",
                                cache->name, entry->id);
#endif
		if(cache->sync)
		{
#ifdef CACHE_DEBUG
			cprintf("%s cache: syncing data to system.\n",
				cache->name);
#endif
			cache->sync(ptr, entry->id, cache);
		} else {
#ifdef CACHE_DEBUG
			cprintf("%s cache: sync is disabled.\n", 
				cache->name);
#endif
		}
	} else {
#ifdef CACHE_DEBUG
		cprintf("%s cache: object dereferenced: %d\n", 
			cache->name, entry->id);
#endif
	}

	return result;
}

int cache_dereference(void* ptr, struct cache* cache)
{
	slock_acquire(&cache->lock);
	int result = cache_dereference_nolock(ptr, cache);

	slock_release(&cache->lock);
	return result;
}

static void* cache_search_nolock(int id, struct cache* cache)
{
	void* result = NULL;

	int x;
	for(x = 0;x < cache->entry_count;x++)
	{
		if(!cache->check(cache->entries[x].slab, id, cache))
		{
			result = cache->entries[x].slab;
			if(cache->entries[x].references <= 0)
				cache->entries[x].references = 1;
			else cache->entries[x].references++;
			break;
		}
	}

#ifdef CACHE_DEBUG
	if(result) cprintf("%s cache: search success.\n", cache->name);
	else cprintf("%s cache: search failure.\n", cache->name);
#endif

	return result;
}

void* cache_search(int id, struct cache* cache)
{
	slock_acquire(&cache->lock);
	void* result = cache_search_nolock(id, cache);
	slock_release(&cache->lock);
	return result;
}

void* cache_addreference(int id, struct cache* cache)
{
	void* result = NULL;
        slock_acquire(&cache->lock);
        /* First search */
        if(!(result = cache_search_nolock(id, cache)))
        {
                /* Not already cached. */
                result = cache_alloc(id, cache);
		/* Do not populate. */
        }
        slock_release(&cache->lock);
        return result;
}

void* cache_reference(int id, struct cache* cache)
{
	void* result = NULL;
	slock_acquire(&cache->lock);
	/* First search */
	if(!(result = cache_search_nolock(id, cache)))
	{
		/* Not already cached. */
		result = cache_alloc(id, cache);
		/* Call the populate function */
		if(cache->populate)
		{
			/* Populate the entry */
			if(cache->populate(result, id, cache->context))
			{
				/* The resource is unavailable. */
				cache_force_free(result, cache);
				result = NULL;
			}
		} else {
			cprintf("%s cache: no populate function assigned.\n",
				cache->name);
		}
	}
	slock_release(&cache->lock);
	return result;
}	

void* cache_soft_search(int id, struct cache* cache)
{
	slock_acquire(&cache->lock);
	void* result = NULL;

	int x;
	for(x = 0;x < cache->entry_count;x++)
	{
		if(!cache->check(cache->entries[x].slab, id, cache))
		{
			result = cache->entries[x].slab;
			break;
		}
	}

	slock_release(&cache->lock);
	return result;
}
