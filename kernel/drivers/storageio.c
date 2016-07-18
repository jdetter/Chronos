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
#include "drivers/storageio.h"

int storageio_read(void* dst, fileoff_t start, size_t sz, 
		struct FSDriver* driver)
{
	if(sz == 0) return 0;
	start += (driver->fs_start << driver->driver->sectshifter);
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

	if(storageio_readsects(startsect + 1, middlesectors, dst_c + bytes,
			sz - bytes, driver->driver))
		return -1;

	/* Do we need to read the last sector? */
	if(bytes == sz) return sz;

	/* Read the last sector */
	if(driver->driver->readsect(endsect, sector, sectsize, driver->driver))
		return -1;

	memmove(dst_c + bytes, sector, (start + sz) & mask);

	return sz;
}

int storageio_write(void* src, fileoff_t start, size_t sz, 
		struct FSDriver* driver)
{
	return 0;
}

int storageio_readsects(sect_t start_sect, int sectors, void* dst, 
		size_t sz, struct StorageDevice* device)
{
	if(device->readsects && sectors > 1)
	{
		if(device->readsects(start_sect, sectors, dst, sz, device))
			return -1;
	} else {
		if(!device->readsect) return -1;

		int x;
		int pos = 0;
		for(x = 0;x < sectors;x++)
		{
			if(device->readsect(start_sect + x, (char*)dst + pos, 
						sz - pos, device))
				return -1;

			pos += device->sectsize;
		}

	}

	return 0;
}

int storageio_writesects(sect_t start_sect, int sectors, void* src,
		size_t sz, struct StorageDevice* device)
{
	if(device->writesects)
	{
		if(device->writesects(start_sect, sectors, src, sz, device))
			return -1;
	} else {
		if(!device->writesect) return -1;

		int x;
		int pos = 0;
		for(x = 0;x < sectors;x++)
		{
			if(device->writesect(start_sect + x, (char*)src + pos, 
						sz - pos, device))
				return -1;

			pos += device->sectsize;
		}
	}

	return 0;
}
