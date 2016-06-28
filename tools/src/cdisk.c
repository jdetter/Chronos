#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/fcntl.h>

#include "mbr.h"

#define VERSION_MINOR 0
#define VERSION_MAJOR 1

#define MAX_BOOT1 446

static void usage(int ret, const char* err, 
		const char* argument) __attribute__ ((noreturn));

static void usage(int ret, const char* err, const char* argument)
{
	printf("\nUsage: cdisk [options] output\n\n");
	printf("Options:\n\n");
	printf("\t--partN=startsect,sectors,image.img{,type}\n");
	printf("\t\tWhere N is a primary partition number (1, 2, 3, 4).\n");
	printf("\t\tstartsect is where this partition starts on disk\n");
	printf("\t\tand sectors is the amount of sectors in the\n");
	printf("\t\tpartition. image.img is the image for the partition.\n");
	printf("\t\tIf the image is not large enough to fill the\n");
	printf("\t\tpartition, the rest of the partition is zeroed.\n");
	printf("\t\t/dev/zero can be used as input to zero the entire\n");
	printf("\t\tpartition. A type can also be passed, which must\n");
	printf("\t\tbe one of the following: EXT2, EXT3, EXT4, FAT,\n");
	printf("\t\tFAT32, NTFS or SWAP.\n\n");
	printf("\t-n sectors\n");
	printf("\t\tThis option allows you to specify the amount of\n");
	printf("\t\tsectors the disk has.\n\n");
	printf("\t-s disksize\n");
	printf("\t\tThis option allows you to specify the size of the\n");
	printf("\t\tdisk in bytes. M and G can also suffix the number\n");
	printf("\t\tto specify either megabytes (1024 bytes) or gigabytes\n");
	printf("\t\t(1024 megabytes).\n\n");
	printf("\t-l sectsize\n");
	printf("\t\tThis options allows you to specify the size of a\n");
	printf("\t\tsingle disk sector. This option is REQUIRED This\n"); 
	printf("\t\tnumber must be one of: 512, 1024, 2048 or 4096\n\n");
	printf("\t-b boot.img\n");
	printf("\t\tThis option can be used to supply primary boot\n");
	printf("\t\tcode. The code must be less than 446 bytes.\n\n");
	printf("\t--stage-2=boot-stage2.elf,start-sector,sector-count\n");
	printf("\t\tThis option can be used to supply second stage\n");
	printf("\t\tboot code. The second stage will be placed at the\n");
	printf("\t\tstart sector with the given amount of sectors. This\n");
	printf("\t\tdoes NOT us up an entry in the partition table.\n\n");
	printf("\t-h or --help\n");
	printf("\t\tShow this help menu.\n\n");
	printf("\t--version\n");
	printf("\t\tShow version information\n\n");

	if(err && argument)
		printf("%s: %s\n\n", err, argument);

	exit(ret);
}

static void version()
{
	printf("cdisk version %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
	printf("Disk image partitioner\n");
	printf("Written by John Detter <jdetter@wisc.edu>\n\n");
	exit(0);
}

static int strto32(const char* str, uint32_t* result)
{
	/* autodetect base */
	const char* parse = str;
	int base = 10;
	if(str[0] == '0' && str[1] == 'x')
	{
		base = 16;
		parse += 2;
	}
	else if(*str == '0')
	{
		base = 8;
		parse += 1;
	}

	/* Parse the number */
	uint32_t ret;
	char* endptr = NULL;
	if(sizeof(int) == sizeof(uint32_t))
	{
		if(sizeof(long) > sizeof(uint32_t))
		{
			uint64_t ret_long;
			ret_long = strtol(parse, &endptr, base);
			ret = (uint32_t)ret_long;
		} else ret = (uint32_t)strtol(parse, &endptr, base);
	} if(sizeof(long) == sizeof(uint32_t))
	{
		ret = (uint32_t)strtol(parse, &endptr, base);
	} else if(sizeof(long long) == sizeof(uint32_t))
	{
		ret = (uint32_t)strtoll(parse, &endptr, base);
	} else {
		long ret_long;
		ret_long = strtol(parse, &endptr, base);
		ret = (uint32_t)ret_long;
	}

	if(str == endptr || endptr == NULL)
		return -1;

	int len = endptr - str;
	if(len < 0 || len > 128)
		return -1;

	*result = ret;
	return len;
}

static int strto64(const char* str, uint64_t* result)
{
    /* autodetect base */
    const char* parse = str;
    int base = 10;
    if(str[0] == '0' && str[1] == 'x')
    {
        base = 16;
        parse += 2;
    } else if(*str == '0')
    {
        base = 8;
        parse += 1;
    }

    /* Parse the number */
    uint64_t ret;
    char* endptr = NULL;
    if(sizeof(long) == sizeof(uint64_t))
    {
        ret = (uint64_t)strtol(parse, &endptr, base);
    } else if(sizeof(long long) == sizeof(uint64_t))
    {
        ret = (uint64_t)strtoll(parse, &endptr, base);
    } else assert(0);

    if(str == endptr || endptr == NULL)
        return -1;

    int len = endptr - str;
    if(len < 0 || len > 128)
        return -1;

    *result = ret;
    return len;
}

static int file_transfer(int infd, int outfd, uint64_t* s, int zero_fill)
{
	uint64_t size = *s;
	/* 1MB transfer buffer */
	int buffer_sz = 0x1000000;
	char* buffer = malloc(buffer_sz);

	int read_EOF = 0;

	uint64_t transferred = 0;
	while(transferred < size)
	{
		int remaining = size - transferred;
		if(remaining > buffer_sz)
			remaining = buffer_sz;

		if(!read_EOF)
		{
			int r = 0;
			while(r < remaining)
			{
				int res = read(infd, buffer + r, remaining - r);
				if(res == 0)  /* EOF */
				{
					read_EOF = 1;
					break;
				}

				if(res < 0) /* Transfer error */
				{
					free(buffer);
					return -1;
				}

				r += res;
			}

			/* How much did we actually read? */
			remaining = r;
		} else {
			if(zero_fill)
			{
				memset(buffer, 0, remaining);
			} else  {
				free(buffer);
				return -1;
			}
		}


		int w = 0;
		while(w < remaining)
		{
			int res = write(outfd, buffer + w, remaining - w);
			
			if(res <= 0)
			{
				/* Disk full? */
				free(buffer);
				return -1;
			}

			w += res;
		}

		/* remiaining is how much we actually got */
		transferred += remaining;
	}

	free(buffer);
	/* success */
	return 0;
}

static int readall(int fd, void* dst, size_t sz)
{
	int r = 0;
	while(r < sz)
	{
		int ret = read(fd, (char*)dst + r, sz - r);

		if(ret == 0) /* EOF */
			return r;
		if(ret < 0)
			return -1;


		r += ret;
	}

	return sz;
}

static int writeall(int fd, void* src, size_t sz)
{
    int w = 0;
    while(w < sz)
    {
        int ret = write(fd, (char*)src + w, sz - w);

        if(ret == 0) /* EOF */
            return w;
        if(ret < 0)
            return -1;

        w += ret;
    }

    return sz;
}

typedef uint32_t sect_t;
typedef uint32_t sectcnt_t;

struct partinfo
{
	sect_t startsect;
	sectcnt_t sectors;
	const char* img;
	int type;
};

struct boot_stage2
{
	char* file;
	sect_t start_sector;
	sectcnt_t sectors;
};

int main(int argc, char** argv)
{
	struct partinfo partitions[4];
	memset(partitions, 0, sizeof(struct partinfo) * 4);
	int sect_size = 0; /* The size of a single sector */
	uint64_t disk_size = 0; /* The size of the entire disk, in bytes */
	sectcnt_t sector_count = 0; /* The amount of sectors */
	char* output_file = NULL;
	char* boot1 = NULL;
	struct boot_stage2 stage2;
	int stage2_setup = 0;
	memset(&stage2, 0, sizeof(struct boot_stage2));

	if(argc <= 1)
		usage(1, NULL, NULL);

	int x;
	for(x = 1;x < argc;x++)
	{
		switch(argv[x][0])
		{
			case '-':
				switch(argv[x][1])
				{
					case '-':
						if(!strcmp(argv[x], "--version"))
							version();
						else if(!strncmp(argv[x] + 2, "part", 4))
						{
							/* Specifies a partiton */
							int part = atoi(argv[x] + 6);
							if(part < 1 || part > 4)
								usage(1, "Invalid partition number", argv[x]);
							part -= 1;

							struct partinfo tmppart;
							/* Get the start sector */
							int pos = 8;
							int ret = -1;
							/* Get the entire length of the string */
							int length = strlen(argv[x]);

							/* Parse the start sector */
							if(pos >= length)
								usage(1, "Illegal partition format", argv[x]);
							ret = strto32(argv[x] + pos, &tmppart.startsect);
							if(ret < 0)
								usage(1, "Illegal partition format", argv[x]);
							pos += ret + 1;
							
							/* Parse the sector count */	
							if(pos >= length)
								usage(1, "Illegal partition format", argv[x]);
							ret = strto32(argv[x] + pos, &tmppart.sectors);
                            if(ret < 0)
                                usage(1, "Illegal partition format", argv[x]);
                            pos += ret + 1;

							/* Get the file name of the image */
							if(pos >= length)
								usage(1, "Illegal partition format", argv[x]);
							tmppart.img = strdup(argv[x] + pos);
							char* search = (char*)tmppart.img;
							int hasType = 0;

							/* find the comma (if it exists) */
							for(;;search++)
							{
								if(*search == ',')
								{
									/* Comma exists, we have a type */
									*search = 0;
									hasType = 1;
									pos += search - tmppart.img + 1;
									break;
								} else if(*search == 0)
								{
									/* There is no comma */
									break;
								}
							}

							/* Try to open the file */
							int fd = open(tmppart.img, O_RDONLY);
							if(fd < 0)
								usage(1, "Access Denied", tmppart.img);
							else close(fd);

							if(hasType)
							{
								char* type = argv[x] + pos;
								if(!strcmp(type, "EXT2"))
									tmppart.type = 0x83;
								else if(!strcmp(type, "EXT3"))
									tmppart.type = 0x83;
								else if(!strcmp(type, "EXT4"))
									tmppart.type = 0x83;
								else if(!strcmp(type, "FAT"))
									tmppart.type = 0x01;
								else if(!strcmp(type, "FAT32"))
									tmppart.type = 0x0B;
								else if(!strcmp(type, "NTFS"))
									tmppart.type = 0x07;
								else if(!strcmp(type, "SWAP"))
									tmppart.type = 0x82;
								else usage(1, "Unknown file system", type);
							} else tmppart.type = 0x00;

							memcpy(partitions + part, &tmppart,
								sizeof(struct partinfo));
						} else if(!strncmp(argv[x], "--stage-2", 9))
						{
							if(stage2_setup)
								usage(1, "You have already specified a stage2",
										argv[x]);

							/* there should be 2 commas */
							char* file_start = NULL;
							char* start_sector_start = NULL;
							char* sectors_start = NULL;

							char* search = argv[x] + 10;
							char* last_start = search;

							for(;*search;search++)
							{
								if(*search == ',')
								{
									if(!file_start)
										file_start = last_start;
									else if(!start_sector_start)
										start_sector_start = last_start;
									else usage(1, "Illegal boot stage 2 format",
											argv[x]);

									last_start = search + 1;
									*search = 0;
								}
							}

							if(sectors_start)
								usage(1, "Illegal boot stage 2 format", argv[x]);
							else sectors_start = last_start;

							if(!sectors_start)
								usage(1, "Illegal boot stage 2 format", argv[x]);

							stage2.file = strdup(file_start);
							printf("Boot stage 2 file: %s\n",
									stage2.file);
							if(strto32(start_sector_start, 
									&stage2.start_sector) < 0)
								usage(1, "Illegal boot stage 2 format", argv[x]);
							printf("Sector start: %llu\n", 
									(long long unsigned int)
									stage2.start_sector);
							if(strto32(sectors_start, 
									&stage2.sectors) < 0)
								usage(1, "Illegal boot stage 2 format", argv[x]);
							printf("Sector count: %llu\n",
									(long long unsigned int)
									stage2.sectors);

							stage2_setup = 1;
						} else {
							usage(1, "Unknown option", argv[x]);	
						}

						break;

					case 'b':
						if(boot1)
							usage(1, "Boot stage 1 has already been supplied",
									argv[x + 1]);
						else boot1 = argv[x + 1];
						x++;
						break;
					case 'n':
						if(strto32(argv[x + 1], &sector_count) < 0)
							usage(1, "Illegal number of sectors", argv[x + 1]);
						x++;
						break;
					case 's':
						if(strto64(argv[x + 1], &disk_size) < 0)
							usage(1, "Illegal disk size", argv[x + 1]);
						x++;
						break;
					case 'l':
						sect_size = atoi(argv[x + 1]);
						x++;
						break;
					default:
						usage(1, "Unknown option", argv[x]);
						break;
				}
				break;
			default:
				if(output_file)
					usage(1, "Output file already specified", "");
				output_file = argv[x];
				break;
		}
	}

	/* validate state */
	if(sect_size == 0)
	{
		printf("Sector size not set!\n");
		return 1;
	}

	if(sector_count && disk_size)
	{
		printf("Either disk size (-s) or sector size (-l)\n"
				"can be provided, not both.");
		return 1;
	}

	if(sector_count && !disk_size)
	{
		/* calculate disk_size */
		disk_size = sector_count * sect_size;
	}

	if(sect_size != 512 && sect_size != 1024 
			&& sect_size != 2048 && sect_size != 4096)
	{
		printf("Invalid sector size: %d\n", sect_size);
		return 1;
	}

	if(!disk_size)
	{
		printf("Sector count or disk size not set!\n");
		return 1;
	}

	if(!sector_count && disk_size < sect_size)
	{
		printf("Illegal disk size in bytes: %llu\n", 
				(long long unsigned int)disk_size);
		return 1;
	}

	if(!output_file)
	{
		printf("No output file was specified.\n");
		return 1;
	}

	/* Make sure disk_size fits all of the partitions */
	uint64_t max_end = 0;
	int max_partition = -1;
	for(x = 0;x < 4;x++)
	{
		uint64_t end = (partitions[x].startsect
			+ partitions[x].sectors) * sect_size;
		if(end > max_end)
		{
			max_end = end;
			max_partition = x;
		}
	}

	if(disk_size < max_end)
	{
		max_partition++;
		printf("\n\n\nWarning: partition %d goes past the end of the disk.\n",
				max_partition);
		printf("You have specified the disk to be %lluK in size, however\n",
				(long long unsigned int)(disk_size / 1024));
		printf("your partition %d ends at %lluK.\n", max_partition,
			   (long long unsigned int)(max_end / 1024));

		printf("\nWould you like to automatically expand the disk? ");
		fflush(stdout);

		char response[512];
		memset(response, 0, 512);
		read(1, response, 511);

		if(!strncmp(response, "Y", 1) || !strncmp(response, "y", 1))
		{
			/* Fix the problem */
			disk_size = max_end;
		} else {
			printf("Please recompute your partition table entries.\n");
			return 1;
		}

		printf("Disk size has been adjusted to %llu\n", 
				(unsigned long long)disk_size);
	}

	int out_fd = open(output_file, O_CREAT | O_WRONLY, 0644);
	if(out_fd < 0)
	{
		perror("cdisk");
		return 1;
	}

	printf("Opened output file %s\n", output_file);
	int partition_files[4];
	for(x = 0;x < 4;x++)
	{
		if(partitions[x].type)
		{
			partition_files[x] = open(partitions[x].img, O_RDONLY);
			if(partition_files[x] == -1)
			{
				perror("cdisk");
				exit(1);
			}

			printf("Opened partition image %s\n", partitions[x].img);
		} else {
			printf("Warning: partition %d unused.\n", x + 1);
			partition_files[x] = -1;
		}
	}

	int dev_zero = open("/dev/zero", O_RDONLY);
	if(dev_zero < 0)
	{
		printf("Couldn't open /dev/zero!\n");
		perror("cdisk");
		return 1;
	}

	/* Create the disk */
	printf("Allocating space for output file...\n");
	if(file_transfer(dev_zero, out_fd, &disk_size, 1))
	{
		printf("Couldn't allocate space for file!\n");
		perror("cdisk");
		return 1;
	}

    /* Check for boot stage 1 */
    int boot1_fd = -1;
	size_t boot1_sz = 0;
    if(boot1)
    {
        printf("Opening boot stage 1 file %s\n", boot1);
        boot1_fd = open(boot1, O_RDONLY);
        if(boot1_fd < 0)
        {
            printf("Couldn't open boot stage 1 file!\n");
            perror("cdisk");
            return 1;
        }

		off_t end = lseek(boot1_fd, 0, SEEK_END);
		if(end > MAX_BOOT1)
		{
			printf("ERROR: your boot stage 1 is too large!\n");
			printf("actual size: %d maximum allowed: %d\n",
					(int)end, MAX_BOOT1);
			return 1;
		}

		boot1_sz = (size_t)end;

		/* Go back to the start */
		lseek(boot1_fd, 0, SEEK_SET);
    }

    /* Check for boot stage 2 */
    int boot2_fd = -1;
	uint64_t boot2_sz = 0;
    if(stage2_setup)
    {
        printf("Opening boot stage 2 file %s\n",
                stage2.file);
        boot2_fd = open(stage2.file, O_RDONLY);

        if(boot2_fd < 0)
        {
            printf("Couldn't open boot stage 2 file!\n");
            perror("cdisk");
            return 1;
        }

		/* Get the size */
		boot2_sz = (uint64_t)lseek(boot2_fd, 0, SEEK_END);
    }

	/* Do the partitions */
	for(x = 0;x < 4;x++)
	{
		if(partitions[x].type <= 0)
		{
			printf("Skipping partition %d...\n", x + 1);
			continue;
		}

		uint64_t start = partitions[x].startsect * sect_size;
		uint64_t size = partitions[x].sectors * sect_size;

		if(lseek64(out_fd, start, SEEK_SET) < 0)
		{
			perror("cdisk");
			return 1;
		}

		printf("Writing partition %d...\n", x + 1);
		if(file_transfer(partition_files[x], out_fd, &size, 1))
		{
			printf("Failed to write partition %d.\n", x + 1);
			return 1;
		}
	}

	/* Do the boot stage 2 code */
	if(boot2_fd > 0)
	{
		printf("Writing boot stage 2...\n");
		/* Seek to start of file */
		if(lseek(boot2_fd, 0, SEEK_SET) != 0)
		{
			perror("lseek");
			return 1;
		}

		/* Seek the output file */
		off_t stage2_start = stage2.start_sector * sect_size;
		if(lseek(out_fd, stage2_start, SEEK_SET) != stage2_start)
		{
			perror("lseek");
			return 1;
		}

		/* Write boot stage 2*/
		if(file_transfer(boot2_fd, out_fd, &boot2_sz, 0))
		{
			printf("Failed to copy boot stage 2.\n");
			return 1;
		}

		printf("Wrote boot stage 2.\n");
	}

	printf("Updating master boot record...\n");
	struct master_boot_record mbr;
	memset(&mbr, 0, sizeof(struct master_boot_record));


	/* Read the bootstrap */
	if(boot1_fd > 0)
	{
		if(lseek(boot1_fd, 0, SEEK_SET) != 0)
		{
			perror("lseek");
			return 1;
		}

		int ret;
		ret = readall(boot1_fd, mbr.bootstrap, boot1_sz);
		if(ret < 0)
		{
			perror("read");
			return 1;
		} else if(ret != boot1_sz)
		{
			/* We couldn't read in the boot code. */
			printf("Read fault during boot code read.\n");
			return 1;
		}
	}

	/* Set the partitions in the mbr */
    for(x = 0;x < 4;x++)
    {
        mbr.table[x].type = partitions[x].type;

        if(partitions[x].type)
        {
            mbr.table[x].start_sector = partitions[x].startsect;
            mbr.table[x].sectors = partitions[x].sectors;
        }
    }

	/* Set the signature */
	mbr.signature = 0xAA55;

	/* Write the MBR to disk */
	if(lseek(out_fd, 0, SEEK_SET) != 0)
	{
		perror("lseek");
		return 1;
	}

	if(writeall(out_fd, &mbr, sizeof(struct master_boot_record)) !=
				sizeof(struct master_boot_record))
	{
		/* We couldn't write the mbr */
		printf("Write fault during mbr write.\n");
		return 1;
	}

	printf("Success.\n");

	return 0;
}
