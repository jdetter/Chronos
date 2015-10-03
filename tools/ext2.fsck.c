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
#include "diskcache.h"
#include "diskio.h"

/* Define a memory based fs driver */

int fd;

struct FSDriver fs;
struct FSHardwareDriver driver;
#define FS_CACHE_SZ 0x100000
#define FS_INODE_SZ 0x10000
#define PGSIZE 4096
uchar fs_cache[FS_CACHE_SZ] __attribute__((aligned(0x1000)));
uchar inode_cache[FS_INODE_SZ] __attribute__((aligned(0x1000)));

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

	return 0;
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

	return 0;
}

int ext2_readblock(void* dst, uint block, struct FSDriver* driver)
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

	return 0;
}

int ext2_writeblock(void* src, uint block, struct FSDriver* driver)
{
        if(lseek(fd, driver->blocksize * block, SEEK_SET) !=
                        driver->blocksize * block)
        {
                printf("Block write seek failure: %d\n",
                        driver->blocksize * block);
                exit(1);
        }

        if(write(fd, src, driver->blocksize) != driver->blocksize)
        {
                printf("Block write failure.\n");
                exit(1);
        }

	return 0;
}

int ext2_readblocks(void* dst, uint block, uint count, 
	struct FSDriver* driver)
{
	char* dst_c = dst;
	int x;
	for(x = 0;x < count;x++)
	{
		if(ext2_readblock(dst_c, block + x, driver))
			return -1;
		dst_c += driver->driver->sectsize;
	}
	return 0;
}

int ext2_writeblocks(void* src, uint block, uint count, 
        struct FSDriver* driver)
{
        char* src_c = src;
        int x;
        for(x = 0;x < count;x++)
        {
                if(ext2_writeblock(src_c, block + x, driver))
                        return -1;
                src_c += driver->driver->sectsize;
        }
        return 0;
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

		char file_path[512];
		strncpy(file_path, path, 512);
		file_path_dir(file_path, 512);
		strncat(file_path, dir.d_name, 512);
		void* file_inode = fs.open(file_path, fs.context);

		if(!file_inode)
		{
			printf("Could not open file: %s\n", file_path);
			continue;
		}
		if(fs.stat(file_inode, &st, fs.context)) return;
		memset(perm_str, 0, 64);
		get_perm(&st, perm_str);
		printf("%s %d %s\n", perm_str, (int)st.st_size, dir.d_name);
		if(fs.close(file_inode, fs.context)) return;
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

	int read_size = 4096;
	int pos = 0;
	char buff[read_size + 1];
	buff[read_size] = 0;
	while(pos < st.st_size)
	{
		int read = fs.read(inode, buff, pos, read_size, fs.context);
		printf("%s", buff);
		pos += read;
	}

	fs.close(inode, fs.context);
	return 0;
}

int touch(char* path)
{
	return fs.create(path, 0644, 0, 0, fs.context);
}

int echo(char* file)
{
	char* message = "This is a short message.\n";
	void* ino = fs.open(file, fs.context);
	if(!ino)
		fs.create(file, 0644, 0x0, 0x0, fs.context);

	fs.write(ino, message, 0, strlen(message) + 1, fs.context);
	fs.close(ino, fs.context);
	return 0;
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

int fsck_mkdir(char* path)
{
	int result = fs.mkdir(path, 0755, 0x0, 0x0, fs.context);
	if(result) printf("mkdir failure.\n");
	return result;
}

int fsck_unlink(char* path)
{
	return fs.unlink(path, fs.context);
}

int fsck_truncate(char* path)
{
	void* ino = fs.open(path, fs.context);
	fs.truncate(ino, 0, fs.context);
	fs.close(ino, fs.context);

	return 0;
}

int dump(void)
{
	return cache_dump(&fs.driver->cache);
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
	fs.driver = &driver;
	diskio_setup(&fs);
	fs.driver->readsect = ata_readsect;
	fs.driver->writesect = ata_writesect;
	driver.readsects = NULL;
	driver.writesects = NULL;
	driver.context = NULL;
	fs.readblocks = ext2_readblocks;
	fs.readblock = ext2_readblock;
	fs.writeblocks = ext2_writeblocks;
	fs.writeblock = ext2_writeblock;
	fs.start = start_block;
	int shifter = -1;
	int block_size_tmp = block_size;
	while(block_size_tmp)
	{
		block_size_tmp >>= 1;
		shifter++;
	}
	fs.driver->sectsize = block_size;
	fs.driver->sectshifter = shifter;
	fs.driver->sectmax = sectmax;
	fs.driver->valid = 1;
	fs.driver->context = NULL;

	/* Initilize the disk cache */
	if(cache_init(fs_cache, FS_CACHE_SZ, PGSIZE,
		"Disk Cache", &fs.driver->cache))
	{
		printf("Failed to initilize cache!\n");
		return -1;
	}
	
	disk_cache_hardware_init(fs.driver);

	/* Start the driver */
	if(ext2_init(block_size, inode_cache, FS_INODE_SZ, &fs))
	{
		printf("Failed to initilize ext2 driver!\n");
		return -1;
	}

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
		} else if(!strncmp(comm_buffer, "dump", 4))
		{
			dump();
		} else if(!strncmp(comm_buffer, "echo ", 5))
                {
                        echo(comm_buffer + 5);
                }else if(!strncmp(comm_buffer, "mkdir ", 6))
                {
                        fsck_mkdir(comm_buffer + 6);
                } else if(!strncmp(comm_buffer, "rm ", 3))
                {
                        fsck_unlink(comm_buffer + 3);
                } else if(!strncmp(comm_buffer, "trunc ", 6))
		{
			fsck_truncate(comm_buffer + 6);
		}
	}
	return 0;
}
