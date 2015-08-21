#ifndef _EXT2_H_
#define _EXT2_H_

/**
 * Initilize the ext2 file system driver with the given parameters.
 */
extern int ext2_init(uint superblock_address, uint block_size,
                uint cache_sz, struct FSHardwareDriver* driver,
                struct FSDriver* fs);

#endif
