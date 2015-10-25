/**
 * Author: John Detter <john@detter.com>
 *
 * Functions for reading and writing to specific locations on a hard disk.
 */

#ifdef __LINUX__
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#else
#include <string.h>
#include <stdlib.h>
#include "kern/stdlib.h"
#endif

#include "file.h"
#include "stdlock.h"
#include "kern/types.h"
#include "fsman.h"

static int disk_read(void* dst, uint start, uint sz, 
		struct FSDriver* driver)
{
	start += (driver->start << driver->driver->sectshifter);
	if(sz == 0) return 0;
	uint shifter = driver->driver->sectshifter;
	uint sectsize = driver->driver->sectsize;
	uint mask = sectsize - 1;
	char* dst_c = dst;

	uint startsect = start >> shifter;
	uint endsect = (start + sz) >> shifter;
	
	char sector[sectsize];

	/* Do the first sector */
	if(driver->driver->readsect(sector, startsect, driver->driver))
		return -1;
	
	uint bytes = sectsize - (start & mask);
	/* Check to make sure we need the whole block */
	if(bytes > sz) bytes = sz;

	memmove(dst_c, sector + (start & mask), bytes);

	if(bytes == sz) return sz;

	int middlesectors = endsect - startsect - 1;

	/* Check for writesects support */
	if(driver->driver->writesects)
	{
		/* Much faster option */
		int result = driver->driver->readsects(dst_c + bytes, 
				startsect +1, middlesectors, driver->driver);
		/* Check for success */
		if(result != sectsize * middlesectors) return -1;
		/* increment bytes */
		bytes += result;
	} else {
		/* No writesects support, have to read individually */
		int sect;
		for(sect = 0;sect < middlesectors;sect++)
		{
			if(driver->driver->readsect(dst_c + bytes, 
					sect + startsect, driver->driver))
				return -1;
			else bytes += sectsize;
		}
	}

	/* Do we need to read the last sector? */
	if(bytes == sz) return sz;

	/* Read the last sector */
	if(driver->driver->readsect(sector, endsect, driver->driver))
		return -1;

	memmove(dst_c + bytes, sector, (start + sz) & mask);

	return sz;
}

static int disk_write(void* src, uint start, uint sz, 
		struct FSDriver* driver)
{
        
        return 0;
}

static int disk_read_blocks(void* dst, uint block_start, uint block_count,
		struct FSDriver* driver)
{
	struct FSHardwareDriver* disk = driver->driver;
	char* dst_c = dst;

        uint sect_shift = disk->sectshifter;
        uint block_shift = driver->blockshift;
        if(sect_shift > block_shift) return -1;
	uint block_to_sector = block_shift - sect_shift;
        uint sector_count = block_count << (block_shift - sect_shift);
        uint start_read = driver->start + (block_start << block_to_sector);

        /* Check for multi sector read support */
        if(!disk->readsects)
        {
                /* Does this driver support any type of read? */
                if(!disk->readsect) return -1;
                int x;
                for(x = 0;x < sector_count;x++)
                {
                        if(disk->readsect(dst_c, start_read + x, disk))
                                return -1;
                        dst_c += disk->sectsize;
                }

                return 0; 
        } else return disk->readsects(dst, start_read, sector_count, disk);
}

static int disk_read_block(void* dst, uint block, struct FSDriver* driver)
{
        return disk_read_blocks(dst, block, 1, driver);
}

static int disk_write_blocks(void* src, uint block_start, uint block_count,
                struct FSDriver* driver)
{
	struct FSHardwareDriver* disk = driver->driver;
        char* src_c = src;

        uint sect_shift = disk->sectshifter;
        uint block_shift = driver->blockshift;
        if(sect_shift > block_shift) return -1;
	uint block_to_sector = block_shift - sect_shift;
        uint sector_count = block_count << (block_shift - sect_shift);
        uint start_write = driver->start + (block_start << block_to_sector);

        /* Check for multi sector read support */
        if(!disk->writesects)
        {
                /* Does this driver support any type of read? */
                if(!disk->writesect) return -1;
                int x;
                for(x = 0;x < sector_count;x++)
                {
                        if(disk->writesect(src_c, start_write + x, disk))
                                return -1;
                        src_c += disk->sectsize;
                }

                return 0;
        } else return disk->writesects(src, start_write, sector_count, disk);
}

static int disk_write_block(void* src, uint block, struct FSDriver* driver)
{
        return disk_write_blocks(src, block, 1, driver);
}

void diskio_setup(struct FSDriver* driver)
{
	driver->disk_read = disk_read;
	driver->disk_write = disk_write;

	driver->readblock = disk_read_block;
	driver->readblocks = disk_read_blocks;

	driver->writeblock = disk_write_block;
	driver->writeblocks = disk_write_blocks;
}
