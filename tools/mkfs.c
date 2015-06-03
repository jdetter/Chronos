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

int allocate_directent(vsfs_inode* parent, char* name, uint inode_num);
void write_inode(uint inode_num, vsfs_inode* inode);

char zero[SECTSIZE];
int find_free_inode(void);
int fd;

int add_dir(char* directory, char* path)
{
	printf("Current directory: %s\n", directory);
	printf("Current path: %s\n", path);
	DIR* d;
	struct dirent* dir;
	d = opendir(directory);
	if(d)
	{
		while((dir = readdir(d)) != NULL)
		{
			char* name = dir->d_name;
			if(!strcmp(name, ".")) continue;
			if(!strcmp(name, "..")) continue;
			char name_buffer[1024];
			memset(name_buffer, 0, 1024);
			strncat(name_buffer, directory, 1024);
			strncat(name_buffer, "/", 1024);
			strncat(name_buffer, name, 1024);

			char path_buffer[1024];
			memset(path_buffer, 0, 1024);
			strncat(path_buffer, path, 1024);
			if(strcmp(path, "/"))
				strncat(path_buffer, "/", 1024);
			strncat(path_buffer, name, 1024);

			struct vsfs_inode new_inode;
			vsfs_clear(&new_inode);
			new_inode.perm = 0666;

			struct stat s;
			if(stat(name_buffer, &s) != 0)
			{
				printf("WARNING: fstat error on file: %s\n", name_buffer);
				continue;
			}	
			if(S_ISDIR(s.st_mode))
			{
				new_inode.perm |= 0111;
				new_inode.type = VSFS_DIR;
				
				/* Link the directory */
				vsfs_link(path_buffer, &new_inode);
				printf("Adding directory: %s\n", name);
				add_dir(name_buffer, path_buffer);
			} else if(S_ISREG(s.st_mode))
			{
				printf("Adding file: %s\n", name);
				new_inode.perm = s.st_mode & 511;
				new_inode.type = VSFS_FILE;
				/* Link the file */
				int inode_num = 
					vsfs_link(path_buffer, &new_inode);
				/* Copy data */
				char contents[s.st_size];
				int reg_file = open(name_buffer, O_RDWR);
				if(reg_file == -1)
				{
					printf("No such file: %s\n", name_buffer);
					exit(1);
				}
				if(read(reg_file, contents, s.st_size)
					!= s.st_size)
				{
					printf("Permission Denied: %s\n", 
						name_buffer);
					exit(1);
				}
			
				vsfs_write(&new_inode, 0, s.st_size, contents);
				/* Update metadata */
				write_inode(inode_num, &new_inode);
			} else {
				printf("Unknown file: %s\n", name);
			}
		}
		closedir(d);
	}
	return 0;
}

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


int main(int argc, char** argv)
{
	//char* argv[] = {"", "-i", "128","-s", "262144", "-r", "fs", "fs.img"};
	//argc = 8;
	memset(zero, 0, SECTSIZE);

	if(argc == 1)
	{
		printf("Usage: mkfs -i [inodes] -s [size] -r [root] file\n");
		return -1;
	}

	int inodes = 0;
	int size = 0;
	char* file = NULL;
	char* root = NULL;

	int arg = 1;
	for(arg = 1;arg < argc;arg++)
	{
		if(!strcmp(argv[arg], "-i") && arg + 1 < argc)
		{
			inodes = atoi(argv[arg + 1]);	
			arg++;
		} else if(!strcmp(argv[arg], "-s") && arg + 1 < argc)
		{
			size = atoi(argv[arg + 1]);
			arg++;
		} else if(!strcmp(argv[arg], "-r") && arg + 1 < argc)
		{
			root = argv[arg + 1];
			arg++;
		} else {
			file = argv[arg];
			break;
		}
	}

	if(root == NULL)
	{
		printf("Must specify a root node.\n");
		return -1;
	}

	if(file == NULL)
	{
		printf("Must specify a resulting file.\n");
		return -1;
	}

	if(inodes <= 0 || size <= 0)
	{
		printf("Inodes != 0 and Size != 0\n");
		return -1;
	}


	int inodes_per_block = SECTSIZE / sizeof(vsfs_inode);
	if(SECTSIZE % sizeof(vsfs_inode) != 0)
	{
		printf("Fatal Error: SECTSIZE %% sizeof(vsfs_inode) != 0\n");
		return -1;
	}

	int inode_blocks = (inodes + inodes_per_block - 1) / inodes_per_block;
	if(inodes % inodes_per_block != 0)
	{
		printf("Fatal Error: Increase inodes to %d. [EFFICIENCY]\n",
				((inodes / inodes_per_block) + 1) * inodes_per_block);
		return -1;
	}
	int inode_bitmap = (inodes + (SECTSIZE * 8) - 1) / SECTSIZE / 8;

	int blocks = size / SECTSIZE;
	if(size % SECTSIZE != 0)
	{
		printf("Fatal Error: File system size must be multiple of 512.\n");
		return -1;
	}
	int block_bitmap = (blocks + (SECTSIZE * 8) - 1) / SECTSIZE / 8;


	printf("**Creating VSFS file system**\n");
	printf("inodes: %d\n", inodes);
	printf("blocks: %d\n", blocks);	

	/* Open the file */
	fd = open(file, O_CREAT | O_TRUNC | O_RDWR, 
			S_IRUSR | S_IWUSR | 
			S_IRGRP | S_IWGRP | 
			S_IWOTH | S_IROTH);
	if(fd == -1)
	{
		printf("Permission Denied: %s\n", file);
		return -1;
	}
	/* Write the last block */
	int last_block = 1 + blocks + inode_blocks + block_bitmap + inode_bitmap;
	int x;
	for(x = 0;x < last_block;x++)
	{
		ata_writesect(x, zero);
	}

	/* Create super block*/
	char buffer[512];
	struct vsfs_superblock* super_block = (struct vsfs_superblock*)buffer;
	super_block->dmap = block_bitmap;
	super_block->dblocks = blocks;
	super_block->imap = inode_bitmap;
	super_block->inodes = inodes;
	ata_writesect(0, buffer);

	/* Initilize driver */
	vsfs_init(0);

	/* Create root inode. */
	struct vsfs_inode root_i;
	vsfs_clear(&root_i);
	root_i.perm = 0644;
	root_i.links_count = 1;
	root_i.type = VSFS_DIR;

	int next_free;
	if((next_free = find_free_inode()) != 1)
	{
		printf("Error: file system is not empty. %d\n", next_free);
		return -1;
	}

	allocate_directent(&root_i, ".", 1);
	allocate_directent(&root_i, "..", 1);
	write_inode(1, &root_i);	
	add_dir(root, "/");

	printf("MKFS finished successfully.\n");

	return 0;
}
