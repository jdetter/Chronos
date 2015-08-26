#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
                printf("Sector read seek failure: %d\n",
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

void get_perm(struct stat* st, char* dst)
{
        /* Start with type of device */
        switch(st->st_mode & S_IFMT)
        {
                case S_IFREG:
                        dst[0] = '-';
                        break;
                case S_IFDIR:
                        dst[0] = 'd';
                        break;
                case S_IFCHR:
                        dst[0] = 'c';
                        break;
                case S_IFBLK:
                        dst[0] = 'b';
                        break;
                case S_IFLNK:
                        dst[0] = 'l';
                        break;
                case S_IFIFO:
                        dst[0] = 'f';
                        break;
                case S_IFSOCK:
                        dst[0] = 's';
                        break;
                default:
                        dst[0] = '?';
                        break;
        }

        if(st->st_mode & PERM_URD)
                dst[1] = 'r';
        else dst[1] = '-';
        if(st->st_mode & PERM_UWR)
                dst[2] = 'w';
        else dst[2] = '-';
        if(st->st_mode & PERM_UEX)
                dst[3] = 'x';
        else dst[3] = '-';
        /* Check for setuid */
        if((st->st_mode & PERM_UEX) && (st->st_mode & S_ISUID))
                dst[3] = 's';

        if(st->st_mode & PERM_GRD)
                dst[4] = 'r';
        else dst[4] = '-';
        if(st->st_mode & PERM_GWR)
                dst[5] = 'w';
        else dst[5] = '-';
        if(st->st_mode & PERM_GEX)
                dst[6] = 'x';
        else dst[6] = '-';
        /* Check for setgid */
        if((st->st_mode & PERM_GEX) && (st->st_mode & S_ISGID))
                dst[6] = 's';

        if(st->st_mode & PERM_ORD)
                dst[7] = 'r';
        else dst[7] = '-';
        if(st->st_mode & PERM_OWR)
                dst[8] = 'w';
        else dst[8] = '-';
        if(st->st_mode & PERM_OEX)
                dst[9] = 'x';
        else dst[9] = '-';
}

void ls(char* path)
{
	void* inode = fs.open(path, fs.context);
	char perm_str[64];
	struct dirent dir;
	struct stat st;
	
	int offset = 0;
	int x;
	for(x = 0;;x++)
	{
		memset(&dir, 0, sizeof(struct dirent));
		int result = fs.getdents(inode, &dir, 1, offset, fs.context);
		if(result < 0) 
		{
			printf("Error reading directory.\n");
			return;
		} else if(result == 0) break;
		
		offset += result;
		fs.stat(inode, &st, fs.context);
		memset(perm_str, 0, 64);
		get_perm(&st, perm_str);
		printf("%s %d %s\n", perm_str, (int)st.st_size, dir.d_name);
		
	}	

	fs.close(inode, fs.context);
}

int cat(char* path)
{
	void* inode = fs.open(path, fs.context);
	if(!inode) return -1;
	struct stat st;

	/* Stat the file */
	if(fs.stat(inode, &st, fs.context)) return -1;

	char buff[st.st_size + 1];
	if(fs.read(inode, buff, 0, st.st_size, fs.context) != st.st_size)
		return -1;
	buff[st.st_size] = 0;

	puts(buff);

	fs.close(inode, fs.context);
	return 0;
}

int touch(char* path)
{
	return fs.create(path, 0644, 0, 0, fs.context);
}

int hard_link(char* argument)
{
	char* arg1 = argument;
	char* arg2 = argument;
	for(;*arg2 != ' ' && *arg2 != 0;arg2++);
	if(*arg2 == ' ')
	{
		*arg2 = 0;	
		arg2++;
	}

	return fs.link(arg1, arg2, fs.context);
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
		perror("");
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

	// ext2_test(&fs.context);

	/* Check for command */
	if(command)
	{
		if(arg1 && !strcmp("ls", command))
			ls(arg1);
	}

	char cwd[512];
	char comm_buffer[512];

	memset(cwd, 0, 512);
	memmove(cwd, "/", 1);

	while(1)
	{
		printf("> ");
		fflush(stdout);
		fgets(comm_buffer, 512, stdin);
		comm_buffer[strlen(comm_buffer) - 1] = 0;
		
		if(!strncmp(comm_buffer, "ls", 2))
		{
			ls(cwd);
		} else if(!strncmp(comm_buffer, "cat", 3))
		{
			char* buff = comm_buffer + 4;
			char compose[512];
			if(*buff == '/')
			{
				/* Absolute*/
				strncpy(compose, buff, 512);
			} else {
				memset(compose, 0, 512);
				strncpy(compose, cwd, 512);
				strncat(compose, "/", 512);
				strncat(compose, buff, 512);
			}
			if(cat(compose))
			{
				printf("Cat error.\n");
			}
		} else if(!strncmp(comm_buffer, "cd", 2))
		{
			char* buff = comm_buffer + 3;
			if(*buff == '/')
			{
				/* absolute */
				strncpy(cwd, buff, 512);
			} else {
				char compose[512];
				memset(compose, 0, 512);
				strncpy(compose, cwd, 512);
				strncat(compose, "/", 512);
				strncat(compose, buff, 512);
				strncpy(cwd, compose, 512);
			}
		} else if(!strncmp(comm_buffer, "pwd", 3))
		{
			puts(cwd);
		} else if(!strncmp(comm_buffer, "touch ", 6))
                {
                        touch(comm_buffer + 6);
		} else if(!strncmp(comm_buffer, "test", 4))
                {
                        ext2_test(&fs.context);
                } else if(!strncmp(comm_buffer, "link ", 5))
                {
                        hard_link(comm_buffer + 5);
                }
	}
	return 0;
}
