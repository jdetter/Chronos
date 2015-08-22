#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "stdlock.h"
#include "file.h"
#include "fsman.h"
#include "ext2.h"

/* Define a memory based fs driver */

int fd;

struct FSDriver fs;
struct FSHardwareDriver driver;

int ata_readsect(void* dst, uint sect, struct FSHardwareDriver* driver)
{
        if(lseek(fd, driver->sectsize * sect, SEEK_SET) != 
			driver->sectsize * sect)
        {
                printf("Sectore read seek failure: %d\n",
                        driver->sectsize * sect);
                exit(1);
        }

        if(read(fd, dst, driver->sectsize) != driver->sectsize)
        {
                printf("Sector Read failure.\n");
                exit(1);
        }

        return driver->sectsize;
}

int ata_writesect(void* src, uint sect, struct FSHardwareDriver* driver)
{
        if(lseek(fd, driver->sectsize * sect, SEEK_SET) != 
			driver->sectsize * sect)
        {
                printf("Sector write seek failure: %d\n",
                        driver->sectsize * sect);
                exit(1);
        }

        if(write(fd, src, driver->sectsize) != driver->sectsize)
        {
                printf("Sector write failure.\n");
                exit(1);
        }

        return driver->sectsize;
}

int ata_readblock(void* dst, uint block, struct FSHardwareDriver* driver)
{
        if(lseek(fd, driver->blocksize * block, SEEK_SET) !=
                        driver->blocksize * block)
        {
                printf("Block seek failure: %d\n",
                        driver->blocksize * block);
                exit(1);
        }

        if(read(fd, dst, driver->blocksize) != driver->blocksize)
        {
                printf("Block read failure.\n");
                exit(1);
        }

        return driver->blocksize;
}

int ata_writeblock(void* src, uint block, struct FSHardwareDriver* driver)
{
        if(lseek(fd, driver->blocksize * block, SEEK_SET) !=
                        driver->blocksize * block)
        {
                printf("Sector write seek failure: %d\n",
                        driver->blocksize * block);
                exit(1);
        }

        if(write(fd, src, driver->blocksize) != driver->blocksize)
        {
                printf("Sector write failure.\n");
                exit(1);
        }

        return driver->blocksize;
}

int ext2_test(void* context);
int main(int argc, char** argv)
{
	char* file_system = NULL;
	int start_block = 0;
	int block_size = 512;
	char* command = NULL;
	char* arg1 = NULL;
	char* arg2 = NULL;

	int x;
	for(x = 1;x < argc;x++)
	{
		if(argv[x][0] == '-')
		{
			if(argv[x][1] == 's')
			{
				start_block = atoi(argv[x + 1]);
				x++;
			} else if(argv[x][1] == 'b')
			{
				block_size = atoi(argv[x + 1]);
				x++;
			}
		} else if(!file_system)
			file_system = argv[x];
		else if(!command)
			command = argv[x];
		else if(!arg1)
			arg1 = argv[x];
		else if(!arg2)
			arg2 = argv[x];
		else printf("ext3.fsck: extraneous argument: %s\n", argv[x]);
	}

	if(!file_system)
	{
		printf("No file system supplied.\n");
		return 1;
	}

	fd = open(file_system, O_RDWR);

	if(fd < 0)
	{
		printf("No such file: %s\n", file_system);
		return 1;
	}

	struct stat st;
	if(fstat(fd, &st))
	{
		printf("fstat failure!\n");
		return -1;
	}
	/* Calculate the last valid sector */
	int sectmax = st.st_size / block_size;

	/* Initilize the memory hardware driver */
	driver.readsect = ata_readsect;
	driver.writesect = ata_writesect;
	driver.readblock = ata_readblock;
	driver.writeblock = ata_writeblock;
	int shifter = -1;
	int block_size_tmp = block_size;
	while(block_size_tmp)
	{
		block_size_tmp >>= 1;
		shifter++;
	}
	driver.sectsize = block_size;
	driver.sectshifter = shifter;
	driver.sectmax = sectmax;
	driver.valid = 1;
	driver.context = NULL;

	/* Start the driver */
	ext2_init(start_block, block_size, FS_CACHE_SIZE, &driver, &fs);

	ext2_test(&fs.context);
	return 0;
}
