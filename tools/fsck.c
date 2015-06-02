/**
 * Make a VSFS file system.
 *
 * Usage: mkfs -i [inodes] -s [size] file
 */

#include "types.h"
#undef NULL

#include "vsfs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#define SECTSIZE 512

extern int b_off;
extern int bmap_off;
extern int imap_off;
extern int i_off;

int allocate_directent(vsfs_inode* parent, char* name, uint inode_num);
void write_inode(uint inode_num, vsfs_inode* inode);

char zero[SECTSIZE];
int find_free_inode(void);
int fd;

int ata_readsect(uint sect, void* dst)
{
	if(lseek(fd, SECTSIZE * sect, SEEK_SET) != SECTSIZE * sect)
	{
		printf("Sectore read seek failure: %d\n",
			SECTSIZE * sect);
		exit(1);
	}

	if(read(fd, dst, SECTSIZE) != SECTSIZE)
	{
		printf("Sector Read failure.\n");
		exit(1);
	}

	return SECTSIZE;
}

int ata_writesect(uint sect, void* src)
{
	if(lseek(fd, SECTSIZE * sect, SEEK_SET) != SECTSIZE * sect)
	{
		printf("Sector write seek failure: %d\n",
			SECTSIZE * sect);
		exit(1);
	}

	if(write(fd, src, SECTSIZE) != SECTSIZE)
	{
		printf("Sector write failure.\n");
		exit(1);
	}

	return SECTSIZE;

}

extern struct vsfs_superblock super;

void ls(char* directory)
{
	vsfs_inode dir;
	if(vsfs_lookup(directory, &dir) == 0)
	{
		printf("No such directory: %s\n", directory);
		return;
	}

	int x;
	for(x = 0;x < dir.size / sizeof(struct directent);x++)
	{
		struct directent entry;
		vsfs_read(&dir, 
			x * sizeof(struct directent), 
			sizeof(struct directent),
			&entry);
		printf("%s\t\t%d\n", entry.name, entry.inode_num);
	}

	printf("Files: %d\n", x);
	
}

void cat(char* file)
{
	vsfs_inode file_i;
	if(vsfs_lookup(file, &file_i) == 0)
        {
                printf("No such directory: %s\n", file);
                return;
        }

	printf("File: %s\n", file);
	printf("size: %d\n", file_i.size);
	printf("Direct blocks: \n");
	int x;
	for(x = 0;x < 9;x++) printf("%d: %d\n", x, file_i.direct[x]);

	char file_buffer[file_i.size];
	vsfs_read(&file_i, 0, file_i.size, file_buffer);

	printf("%s\n", file_buffer);
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		printf("Usage: fsck fs.img\n");
		return -1;
	}

	char* file;
	int start = 0;
	int x;
	for(x = 1;x < argc;x++)
	{
		if(!strcmp(argv[x], "-s"))
		{
			x++;
			start = atoi(argv[x]);
		} else {
			file = argv[x];
		}
	}

	fd = open(file, O_RDWR);
	vsfs_init(start);

	char cwd[1024];
	memset(cwd, 0, 1024);
	cwd[0] = '/';

	printf("Inodes: %d\n", super.inodes);
	printf("Inode bitmaps: %d\n", super.imap);
	printf("Blocks: %d\n", super.dblocks);
	printf("Block bitmaps: %d\n", super.dmap);

	printf("imap_off: %d\n", imap_off);
	printf("bmap_off: %d\n", bmap_off);
	printf("i_off: %d\n", i_off);
	printf("b_off: %d\n", b_off);

	char command[512];
	while(1)
	{
		printf("Your current working directory: %s\n", cwd);
		printf("> ");
		fgets(command, 512, stdin);
		command[strlen(command) - 1] = 0;
		char* cptr = command;
		if(!strncmp(command, "cd", 2))
		{
			cptr += 3;
			memset(cwd, 0, 1024);
			strncpy(cwd, cptr, 1024);	
		} else if(!strncmp(command, "ls", 2))
		{
			ls(cwd);
		} else if(!strncmp(command, "cat", 3))
		{
			char path_buff[1024];
			memset(path_buff, 0, 1024);
			strncat(path_buff, cwd, 1024);
			if(strcmp(cwd, "/"))
				strncat(path_buff, "/", 1024);
			strncat(path_buff, command + 4, 1024);
			cat(path_buff);
		}

	}

	return 0;
}
