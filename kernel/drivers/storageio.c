/**
 * Author: John Detter <john@detter.com>
 *
 * Functions for reading and writing to specific locations on a
 * storage device.
 */

#ifdef __LINUX__
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#else
#include <string.h>
#include <stdlib.h>
#include "kstdlib.h"
#endif

#include "file.h"
#include "stdlock.h"
#include "fsman.h"

static int storage_read(void* dst, fileoff_t start, size_t sz, 
		struct FSDriver* driver)
{
	start += (driver->fs_start << driver->driver->sectshifter);
	if(sz == 0) return 0;
	int shifter = driver->driver->sectshifter;
	size_t sectsize = driver->driver->sectsize;
	int mask = sectsize - 1;
	char* dst_c = dst;

	sect_t startsect = start >> shifter;
	sect_t endsect = (start + sz) >> shifter;

	char sector[sectsize];

	/* Do the first sector */
	if(driver->driver->readsect(startsect, sector, sectsize, driver->driver))
		return -1;

	size_t bytes = sectsize - (start & mask);
	/* Check to make sure we need the whole block */
	if(bytes > sz) bytes = sz;

	memmove(dst_c, sector + (start & mask), bytes);

	if(bytes == sz) return sz;

	int middlesectors = endsect - startsect - 1;

	/* Check for readsects support */
	if(driver->driver->readsects)
	{
		/* Much faster option */
		int result = driver->driver->readsects(startsect + 1, 
				middlesectors, dst_c + bytes, sz - bytes,
			   	driver->driver);
		/* Check for success */
		if(result != sectsize * middlesectors) return -1;
		/* increment bytes */
		bytes += result;
	} else {
		/* No writesects support, have to read individually */
		int sect;
		for(sect = 0;sect < middlesectors;sect++)
		{
			if(driver->driver->readsect(sect + startsect,
						dst_c + bytes, sz - bytes, driver->driver))
				return -1;
			else bytes += sectsize;
		}
	}

	/* Do we need to read the last sector? */
	if(bytes == sz) return sz;

	/* Read the last sector */
	if(driver->driver->readsect(endsect, sector, sectsize, driver->driver))
		return -1;

	memmove(dst_c + bytes, sector, (start + sz) & mask);

	return sz;
}

static int storage_write(void* src, fileoff_t start, size_t sz, 
		struct FSDriver* driver)
{
	return 0;
}

static int storage_read_blocks(void* dst, blk_t block_start, int block_count,
		struct FSDriver* driver)
{
	struct StorageDevice* disk = driver->driver;
	char* dst_c = dst;

	int sect_shift = disk->sectshifter;
	int block_shift = driver->blockshift;
	if(sect_shift > block_shift) return -1;
	int block_to_sector = block_shift - sect_shift;
	int sector_count = block_count << (block_shift - sect_shift);
	blk_t start_read = driver->fs_start + (block_start << block_to_sector);

	/* Check for multi sector read support */
	if(!disk->readsects)
	{
		/* Does this driver support any type of read? */
		if(!disk->readsect) return -1;
		int x;
		for(x = 0;x < sector_count;x++)
		{
			if(disk->readsect(start_read + x, dst_c, disk->sectsize, disk))
				return -1;
			dst_c += disk->sectsize;
		}

		return 0; 
	} else return disk->readsects(start_read, sector_count, dst, 
			driver->blocksize, disk);
}

static int storage_read_block(void* dst, blk_t block, struct FSDriver* driver)
{
	return storage_read_blocks(dst, block, 1, driver);
}

static int storage_write_blocks(void* src, blk_t block_start, int block_count,
		struct FSDriver* driver)
{
	struct StorageDevice* disk = driver->driver;
	char* src_c = src;

	int sect_shift = disk->sectshifter;
	int block_shift = driver->blockshift;
	if(sect_shift > block_shift) return -1;
	int block_to_sector = block_shift - sect_shift;
	int sector_count = block_count << (block_shift - sect_shift);
	blk_t start_write = driver->fs_start + (block_start << block_to_sector);

	/* Check for multi sector read support */
	if(!disk->writesects)
	{
		/* Does this driver support any type of read? */
		if(!disk->writesect) return -1;
		int x;
		for(x = 0;x < sector_count;x++)
		{
			if(disk->writesect(start_write + x, src_c, driver->blocksize, disk))
				return -1;
			src_c += disk->sectsize;
		}

		return 0;
	} else return disk->writesects(start_write, sector_count, src, 
			driver->blocksize, disk);
}

static int storage_write_block(void* src, blk_t block, struct FSDriver* driver)
{
	return storage_write_blocks(src, block, 1, driver);
}

void storageio_setup(struct FSDriver* driver)
{
	driver->storage_read = storage_read;
	driver->storage_write = storage_write;

	driver->readblock = storage_read_block;
	driver->readblocks = storage_read_blocks;

	driver->writeblock = storage_write_block;
	driver->writeblocks = storage_write_blocks;
}
