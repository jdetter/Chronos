#ifndef _CACHEMAN_H_
#define _CACHEMAN_H_

struct cache
{
	uint entry_count; /* How many entries / slabs are there? */
	int entry_shift; /* Use shifts instead of multiplication */
        struct cache_entry* entries; /* List of entries */
        char* slabs; /* Pointer to the first slab */
	int slab_shift; /* Quick shift is available for log2(slab)*/
        uint slab_sz; /* How big are the slabs? */
        slock_t lock; /* Lock needed to change the cache */

	/**
	 * Custom comparison function. Decides what gets compared on a
	 * search operation. obj is the object in the cache, id is the
	 * id of the object were searching for.
	 */	
	int (*check)(void* obj, int id, struct cache* cache);

	/**
	 * Sync the object with the underlying system.
	 */
	int (*sync)(void* obj, int id, struct cache* cache);
	
	/* NOTE: external locking MUST be used with these functions. */
	/* Backend caching function (for backend use only) */
	void* (*search)(int id, struct cache* cache);
	void* (*alloc)(int id, int slabs, struct cache* cache);
	int (*dereference)(void* ptr, struct cache* cache);
};

/**
 * Initilize a cache structure. Returns 0 on success, -1 on failure.
 */
int cache_init(void* cache_area, uint sz, uint data_sz, struct cache* cache);

/**
 * Search the cache for any object with the given id. If no such
 * object is found, a new cache object is generated and returned.
 * If there is no space in the cache left, NULL is returned.
 */
void* cache_reference(int id, int slabs, struct cache* cache);

/**
 * Free a pointer that was passed by cache_alloc. Returns the amount
 * of references left on that object. The object is only freed if
 * the amount of references left is 0.
 */
int cache_dereference(void* ptr, struct cache* cache);

/**
 * Search the cache for an object with the given id. Returns NULL on
 * failure, otherwise returns the address of the object. NOTE: this
 * adds a reference to the object! 
 */
void* cache_search(int id, struct cache* cache);

/**
 * Same thing as cache_search except it doesn't touch the reference
 * counter. This can be dangerous if the object gets dereferenced
 * while the program is using it. Use with caution.
 */
void* cache_soft_search(int id, struct cache* cache);

#endif
