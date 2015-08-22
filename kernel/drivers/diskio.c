#ifdef __LINUX__
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#else
#include "stdlib.h"
#endif

#include "file.h"
#include "stdlock.h"
#include "types.h"
#include "fsman.h"

static int disk_read(void* dst, uint start, uint sz, 
		struct FSHardwareDriver* driver)
{
	if(sz == 0) return 0;
	uint shifter = driver->sectshifter;
	uint sectsize = driver->sectsize;
	uint mask = sectsize - 1;
	char* dst_c = dst;

	uint startsect = start >> shifter;
	uint endsect = (start + sz) >> shifter;
	
	char sector[sectsize];

	/* Do the first sector */
	if(driver->readsect(sector, startsect, driver) != sectsize)
		return -1;
	
	uint bytes = sectsize - (start & mask);
	/* Check to make sure we need the whole block */
	if(bytes > sz) bytes = sz;

	memmove(dst_c, sector + (start & mask), bytes);

	if(bytes == sz) return sz;

	int middlesectors = endsect - startsect - 1;

	/* Check for writesects support */
	if(driver->writesects)
	{
		/* Much faster option */
		int result = driver->readsects(dst_c + bytes, 
				startsect +1, middlesectors, driver);
		/* Check for success */
		if(result != sectsize * middlesectors) return -1;
		/* increment bytes */
		bytes += result;
	} else {
		/* No writesects support, have to read individually */
		int sect;
		for(sect = 0;sect < middlesectors;sect++)
		{
			if(driver->readsect(dst_c + bytes, sect, driver) 
					!= sectsize)
				return -1;
			else bytes += sectsize;
		}
	}

	/* Do we need to read the last sector? */
	if(bytes == sz) return sz;

	/* Read the last sector */
	if(driver->readsect(sector, endsect, driver) != sectsize)
		return -1;

	memmove(dst_c + bytes, sector, (start + sz) & mask);

	return sz;
}

static int disk_write(void* src, uint start, uint sz, 
		struct FSHardwareDriver* driver)
{
        
        return 0;
}

void diskio_setup(struct FSHardwareDriver* driver)
{
	driver->read = disk_read;
	driver->write = disk_write;
}
