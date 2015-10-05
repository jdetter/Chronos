#ifndef _EXT2_H_
#define _EXT2_H_

/**
 * Initilize the ext2 file system driver with the given parameters.
 */
extern int ext2_init(uint sect_size, void* icache, uint cache_sz, 
		struct FSDriver* fs);

#endif
