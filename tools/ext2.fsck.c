#include <sys/stat.h>
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

struct FSMemoryContext
{
	int sectsize;
};

struct FSMemoryContext fscontext;

int ata_readsect(void* dst, uint sect, struct FSHardwareDriver* driver)
{
	struct FSMemoryContext* context = driver->context;
        if(lseek(fd, context->sectsize * sect, SEEK_SET) != 
			context->sectsize * sect)
        {
                printf("Sectore read seek failure: %d\n",
                        context->sectsize * sect);
                exit(1);
        }

        if(read(fd, dst, context->sectsize) != context->sectsize)
        {
                printf("Sector Read failure.\n");
                exit(1);
        }

        return 0;
}

int ata_writesect(void* src, uint sect, struct FSHardwareDriver* driver)
{
	struct FSMemoryContext* context = driver->context;
        if(lseek(fd, context->sectsize * sect, SEEK_SET) != 
			context->sectsize * sect)
        {
                printf("Sector write seek failure: %d\n",
                        context->sectsize * sect);
                exit(1);
        }

        if(write(fd, src, context->sectsize) != context->sectsize)
        {
                printf("Sector write failure.\n");
                exit(1);
        }

        return 0;
}

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

	/* Initilize the memory hardware driver */
	driver.read = ata_readsect;
	driver.write = ata_writesect;
	driver.valid = 1;
	driver.context = &fscontext;
	fscontext.sectsize = block_size;

	/* Start the driver */
	ext2_init(start_block, block_size, FS_CACHE_SIZE, &driver, &fs);
	return 0;
}
