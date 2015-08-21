#define __LINUX_DEFS__
#include "types.h"
#undef NULL

#include <sys/types.h>
#include <types.h>
#include "file.h"
#include "stdlock.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "fsman.h"
#include "vsfs.h"

#define SECTSIZE 512

int allocate_directent(vsfs_inode* parent, char* name, uint inode_num);
void write_inode(uint inode_num, vsfs_inode* inode);

char zero[SECTSIZE];
int find_free_inode(void);
int fd;

struct FSDriver fs;
struct FSHardwareDriver driver;

int ata_readsect(void* dst, uint sect, struct FSHardwareDriver* driver)
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

	return 0;
}

int ata_writesect(void* src, uint sect, struct FSHardwareDriver* driver)
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

	return 0;
}

extern struct vsfs_superblock super;

void ls(char* directory)
{
	vsfs_inode dir;
	if(vsfs_lookup(directory, &dir, (struct vsfs_context*)&fs.context) == 0)
	{
		printf("No such directory: %s\n", directory);
		return;
	}

	int x;
	for(x = 0;x < dir.size / sizeof(struct vsfs_directent);x++)
	{
		struct vsfs_directent entry;
		vsfs_read(&dir, &entry, 
			x * sizeof(struct vsfs_directent), 
			sizeof(struct vsfs_directent),
			(struct vsfs_context*)&fs.context);
		printf("%s\t\t%d\n", entry.name, entry.inode_num);
	}

	printf("Files: %d\n", x);
	
}

void cat(char* file)
{
	vsfs_inode file_i;
	if(vsfs_lookup(file, &file_i, (struct vsfs_context*)&fs.context) == 0)
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
	vsfs_read(&file_i, file_buffer, 0, file_i.size, 
		(struct vsfs_context*)&fs.context);

	printf("%s\n", file_buffer);
}

void export(char* path, char* dst)
{
	void* file = fs.open(path, &fs.context);

	if(!file)
	{
		printf("No file to export.\n");
		exit(1);
	} else printf("source file opened.\n");

	struct stat st;
	fs.stat(file, &st, &fs.context);

	int size = st.st_size;
	int output = open(dst, O_RDWR | O_CREAT, 0666);
	if(output < 0)
	{
		printf("No file to output to.\n");
		exit(1);
	} else printf("dst file opened.\n");
	char* cache = malloc(size);
	fs.read(file, cache, 0x0, size, &fs.context);
	write(output, cache, size);
	printf("Cache: %s\n", cache);
}

void cp(char* src, char* path)
{
	int file = open(src, O_RDONLY);
	if(file < 0)
	{
		printf("%s: no such file.\n", src);
		exit(1);
	} else printf("%s opened.\n", src);


	void* old = fs.open(path, &fs.context);
	if(!old)
	{
		printf("Warning: %s did not exist before.\n", path);
	}
	printf("Unlink return value: %d\n", fs.unlink(path, &fs.context));
	
	void* removed = fs.open(path, &fs.context);
	if(removed)
	{
		printf("Removal failed.\n");
		exit(1);
	}
	struct stat st;
	fstat(file, &st);

	fs.create(path, st.st_mode, 0x0, 0x0, &fs.context);
	char* buffer = malloc(st.st_size);
	if(!buffer)
	{
		printf("File too large: %d\n", (int)st.st_size);
		exit(1);
	}
	read(file, buffer, st.st_size);
	void* ino = fs.open(path, &fs.context);
	if(!ino)
	{
		printf("%s: Could not create new file.\n", path);
		exit(1);
	}

	fs.write(ino, buffer, 0, st.st_size, &fs.context);
	free(buffer);
	fs.close(ino, &fs.context);
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		printf("Usage: fsck fs.img\n");
		return -1;
	}

	char* file = NULL;
	char* sub_command = NULL;
	char* arg1 = NULL;
	char* arg2 = NULL;
	int start = 0;
	int x;
	for(x = 1;x < argc;x++)
	{
		if(!strcmp(argv[x], "-s"))
		{
			x++;
			start = atoi(argv[x]);
		} else if(file == NULL)
			file = argv[x];
		else if(sub_command == NULL)
			sub_command = argv[x];
		else if(arg1 == NULL)
			arg1 = argv[x];
		else if(arg2 == NULL)
			arg2 = argv[x];
		else printf("Warning: too many arguments!\n");
	}

	fd = open(file, O_RDWR);
	driver.valid = 1;
	driver.read = ata_readsect;
	driver.write = ata_writesect;
	struct vsfs_context* context = (struct vsfs_context*)fs.context;
	context->hdd = &driver;
	uchar fake_cache[512];
	vsfs_init(start, 0, 512, 512, fake_cache, context);
	vsfs_driver_init(&fs);

	char cwd[1024];
	memset(cwd, 0, 1024);
	cwd[0] = '/';

	if(sub_command && !strcmp(sub_command, "cp"))
	{
		if(arg1 && arg2)
		{
			printf("cp %s %s\n", arg1, arg2);
			fflush(stdin);
			cp(arg1, arg2);
			printf("Patch success.\n");
			exit(0);
		}
	}

	if(sub_command && !strcmp(sub_command, "export"))
	{
		if(arg1 && arg2)
		{
			printf("export %s -> %s\n", arg1, arg2);
			fflush(stdin);
			export(arg1, arg2);
			exit(0);
		}
	}

	printf("Inodes: %d\n", context->super.inodes);
	printf("Inode bitmaps: %d\n", context->super.imap);
	printf("Blocks: %d\n", context->super.dblocks);
	printf("Block bitmaps: %d\n", context->super.dmap);

	printf("imap_off: %d\n", context->imap_off);
	printf("bmap_off: %d\n", context->bmap_off);
	printf("i_off: %d\n", context->i_off);
	printf("b_off: %d\n", context->b_off);

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
