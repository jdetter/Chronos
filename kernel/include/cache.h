#ifndef _CACHE_H_
#define _CACHE_H_

#define CACHE_DEBUG_NAME_LEN 64

struct cache
{
	int entry_count; /* How many entries / slabs are there? */
	int entry_shift; /* Use shifts instead of multiplication */
	struct cache_entry* entries; /* List of entries */
	uintptr_t last_entry; /* the address of the last entry */
	char* slabs; /* Pointer to the first slab */
	int slab_shift; /* Quick shift is available for log2(slab)*/
	size_t slab_sz; /* How big are the slabs? */
	slock_t lock; /* Lock needed to change the cache */
	int clock; /* Points to the last entry allocated */
	char name[CACHE_DEBUG_NAME_LEN]; /* name of the cache (DEBUG) */
	int cache_hits; /* How many times have we gotten a cache hit? */
	int cache_miss; /* How many times have we gotten a cache miss? */

	/**
	 * Custom comparison function. Decides what gets compared on a
	 * search operation. obj is the object in the cache, id is the
	 * id of the object were searching for.
	 */	
	int (*check)(void* obj, int id, struct cache* cache, void* context);

	/**
	 * Sync the object with the underlying system.
	 */
	int (*sync)(void* obj, int id, struct cache* cache, void* context);

	/**
	 * Required populate function. When an object isn't found in the
	 * cache, this populate function will be called so that the resource
	 * can be loaded from the underlying system (usually from disk).
	 * Return 0 on success, otherwise if the resource is unavailable
	 * or does not exist, return a non 0 integer.
	 */
	int (*populate)(void* obj, int id, void* context);

	/**
	 * Optional function to try to help resolve queries. Returns 0
	 * on a query match, -1 otherwise.
	 */
	int (*query)(void* query_obj, void* test_obj, void* context);

	/**
	 * Optional cleanup function. This is called when a cache object
	 * is ejected from the cache.
	 */
	int (*eject)(void* obj, int id, void* context);
};

/**
 * Initilize a cache structure. Returns 0 on success, -1 on failure.
 */
int cache_init(void* cache_area, size_t sz, size_t data_sz,
		char* name, struct cache* cache);

/**
 * Search for the entry in the cache. If not found, do not
 * populate the entry but still return a new cache object. Context
 * is the context of the underlying storage element. If there
 * is no space available in the cache, NULL is returned.
 */
void* cache_addreference(int id, struct cache* cache, void* context);

/**
 * Search the cache for any object with the given id. If no such
 * object is found, a new cache object is generated and returned.
 * If the object needs to be populated, context is the context of
 * the underlying storage. If there is no space in the cache left, 
 * NULL is returned.
 */
void* cache_reference(int id, struct cache* cache, void* context);

/**
 * Search the cache for the given data. This function only works if
 * the application has defined a query function in the cache structure.
 * The object returned is a soft link, it should not be freed and it
 * did not increase the reference cound of the object. If the object
 * is to be used, you should reference the object.
 */
void* cache_query(void* data, struct cache* cache, void* context);

/**
 * When the given reference is fully dereferenced, it will be removed
 * from the cache immediatly. Eject will be called if needed. Returns
 * 0 on success, -1 otherwise.
 */
int cache_set_clobber(void* ptr, struct cache* cache);

/**
 * Check the amount of references there are to a element in the
 * cache. Returns the amount of references that are pointing to
 * the element. Returns -1 on failure.
 */
int cache_count_refs(void* ptr, struct cache* cache);

/**
 * Free a pointer that was passed by cache_alloc. Returns the amount
 * of references left on that object. The object is only freed if
 * the amount of references left is 0.
 */
int cache_dereference(void* ptr, struct cache* cache, void* context);

/**
 * Hint to the cache that this entry might be accessed soon. (major
 * performance increase). Context is the context of the underlying
 * storage system.
 */
void cache_prepare(int id, struct cache* cache, void* context);

/**
 * Dump information on the objects currently in the cache.
 */
int cache_dump(struct cache* cache);

/**
 * Sync the cache with the underlying system.
 */
void cache_sync_all(struct cache* cache, void* context);

/**
 * Sync a specific cache object.
 */
int cache_sync(void* ptr, struct cache* cache, void* context);

/**
 * Eliminate and clear all stale entries (reduces performance but
 * makes it easier to find leaks).
 */
int cache_clean(struct cache* cache, void* context);

/**
 * Calculate the minimum cache size needed for the amount of entries.
 */
int cache_calc_size(int entries, int entry_size);

#endif
