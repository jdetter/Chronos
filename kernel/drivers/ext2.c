#ifdef __LINUX__

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#define uint_64 uint64_t
#define uint_32 uint32_t
#define uint_16 uint16_t
#define uint_8  uint8_t

/* map cprintf to printf */
#define cprintf printf

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

/* need the log2 algorithm */
int log2_linux(uint value)
{
        uint value_orig = value;
        /* Shift to the right until we hit a 1 */
        int x = 0;
        while(value != 1)
        {
                value >>= 1;
                x++;
        }

        /* Check work */
        if(1 << x != value_orig) return -1;

        return x;
}


#define log2(value) log2_linux(value)

#else

#include "types.h"
#include "stdarg.h"
#include "stdlib.h"
#include "panic.h"

#endif

#include "file.h"
#include "stdlock.h"
#define NO_DEFINE_INODE
#include "fsman.h"
#include "ext2.h"
#include "diskio.h"

#define EXT2_DIRECT_COUNT 12

#define EXT2_MAX_PATH FILE_MAX_PATH
#define EXT2_MAX_NAME FILE_MAX_NAME
#define EXT2_MAX_PATH_SEGS 32

struct ext2_linux_specific
{
	uint_8 fragment_number;
	uint_8 fragment_size;
	uint_16 reserved1;
	uint_16 userid_high;
	uint_16 groupid_high;
	uint_32 reserved2;
};

struct ext2_hurd_specific
{
        uint_8 fragment_number;
        uint_8 fragment_size;
        uint_16 mode_high;
        uint_16 userid_high;
        uint_16 groupid_high;
        uint_32 author_id;
};

struct ext2_masix_specific
{
        uint_8 fragment_number;
        uint_8 fragment_size;
	uint_8 reserved[10];
};

struct ext2_disk_inode
{
	uint_16 mode;
	uint_16 owner;
	uint_32 lower_size;
	uint_32 last_access_time;
	uint_32 creation_time;
	uint_32 modification_time;
	uint_32 deletion_time;
	uint_16 group;
	uint_16 hard_links;
	uint_32 sectors;
	uint_32 flags;
	uint_32 os_value_1;
	uint_32 direct[EXT2_DIRECT_COUNT];
	uint_32 indirect;
	uint_32 dindirect;
	uint_32 tindirect;
	uint_32 generation;
	uint_32 acl;
	uint_32 upper_size;
	uint_32 fragment;
	
	union
	{
		struct ext2_linux_specific linux_specific;
		struct ext2_hurd_specific hurd_specific;
		struct ext2_masix_specific masix_specific;
	};
};

struct ext2_cache_inode
{
	uchar allocated;
	struct ext2_disk_inode ino;
	uint_32 inode_num;
	uint_32 references;
	char path[FILE_MAX_PATH];
};

struct ext2_base_superblock
{
	uint_32 inode_count;
	uint_32 block_count;
	uint_32 reseved_count;
	uint_32 free_block_count;
	uint_32 free_inode_count;	
	uint_32 superblock_address;
	uint_32 block_size_shift;
	uint_32 frag_size_shift;
	uint_32 blocks_per_group;
	uint_32 frags_per_group;
	uint_32 inodes_per_group;
	uint_32 last_mount_time;
	uint_32 last_written_time;
	uint_16 consistency_mounts;
	uint_16 max_consistency_mounts;
	uint_16 signature;
	uint_16 state;
	uint_16 error_policy;
	uint_16 minor_version;
	uint_32 posix_consistency;
	uint_32 posix_interval;
	uint_32 os_uuid;
	uint_32 major_version;
	uint_16 owner_id;
	uint_16 group_id;
};

struct ext2_extended_base_superblock
{
	uint_32 first_inode;
	uint_16 inode_size;
	uint_16 my_block_group;
	uint_32 optional_features;
	uint_32 required_features;
	uint_32 readonly_features;
	char 	fs_uuid[16];
	char 	name[16];
	char 	last_mount[64];
	uint_32 compression_alg;
	uint_8 	file_preallocate;
	uint_8 	dir_preallocate;
	uint_16 unused1;
	char 	journal_id[16];
	uint_32 journal_inode;
	uint_32 journal_device;
	uint_32 inode_orphan_head;
	
};

struct ext2_block_group_table
{
	uint_32 block_bitmap_address; /* block usage bitmap address */
	uint_32 inode_bitmap_address; /* inode usage bitmap address */
	uint_32 inode_table; /* Start of the inode table */
	uint_16 free_blocks; /* How many blocks are available? */
	uint_16 free_inodes; /* How many inodes are available? */
	uint_16 dir_count; /* How many directories are in this group? */
};

struct ext2_dirent
{
	uint_32 inode;
	uint_16 size;
	uint_8  name_length;
	uint_8  type;
	char	name[EXT2_MAX_NAME];
};

/* What is the current state of the drive? */
#define EXT2_STATE_CLEAN 		0x01
#define EXT2_STATE_ERROR 		0x02

/* How should we handle errors? */
#define EXT2_ERROR_POLICY_IGNORE	0x01
#define EXT2_ERROR_POLICY_REMOUNT 	0x02
#define EXT2_ERROR_POLICY_PANIC   	0x03

/* Who created this partition? */
#define EXT2_CREATOR_LINUX		0x00
#define EXT2_CREATOR_GNUHURD		0x01
#define EXT2_CREATOR_MASIX		0x02
#define EXT2_CREATOR_FREEBSD		0x03
#define EXT2_CREATOR_OTHERUNIX		0x04

/* Optional feature flags */
#define EXT2_OFEATURE_PREALLOC		0x0001
#define EXT2_OFEATURE_AFS		0x0002
#define EXT2_OFEATURE_EXT3		0x0004
#define EXT2_OFEATURE_EXTENDED		0x0008
#define EXT2_OFEATURE_EXPANDABLE	0x0010
#define EXT2_OFEATURE_HASH		0x0020

/* Required feature flags */
#define EXT2_RFEATURE_COMPRESSION	0x0001
#define EXT2_RFEATURE_DIR_TYPE		0x0002
#define EXT2_RFEATURE_REPLAY		0x0004
#define EXT2_RFEATURE_JOURNAL_DEV	0x0008

/* Read-only feature flags */
#define EXT2_ROFEATURE_SPARSE		0x0001
#define EXT2_ROFEATURE_64BIT_SIZE	0x0002
#define EXT2_ROFEATURE_BINTREE		0x0004

/* Inode flags */
#define EXT2_INODE_SECURE_DELETE	0x00000001
#define EXT2_INODE_COPY_ON_DELETE	0x00000002
#define EXT2_INODE_COMPRESSION		0x00000004
#define EXT2_INODE_SYNC			0x00000008
#define EXT2_INODE_IMMUTABLE		0x00000010
#define EXT2_INODE_APPEND_ONLY		0x00000020
#define EXT2_INODE_NO_DUMP		0x00000040
#define EXT2_INODE_NO_ATIME		0x00000080
#define EXT2_INODE_HASHED		0x00010000
#define EXT2_INODE_AFS_DIR		0x00020000
#define EXT2_INODE_JOURNAL_DATA		0x00040000

typedef struct ext2_disk_inode disk_inode;
typedef struct ext2_cache_inode inode;
typedef struct ext2_context context;

struct ext2_context
{
	/* Some cache information */
	slock_t cache_lock; /* Lock that must acquired to touch the cache*/
	uint cache_count; /* How many inodes are there in memory? */
	inode* cache; /* Inode cache */
	slock_t fs_lock; /* Lock that must be acquired to touch the fs */
	inode* root; /* The root inode */

	/* superblock information */
	struct ext2_base_superblock base_superblock;
	struct ext2_extended_base_superblock extended_superblock;

	/* hardware driver */
	struct FSHardwareDriver* driver;
	/* File system metadata */
	struct FSDriver* fs;	

	/* Quick calculations */
	int inodegroupshift; /* Turn an inode into a block group */
	int inodesizeshift; /* Turn an inode into a table addr */
	int inodesize; /* Size of an inode */
	int revision; /* Revision level we want to use */
	int blocksize; /* The size of data blocks */
	int blockshift; /* The size of data blocks */
	int sectsize; /* The size of sectors on disk */
	int firstgroup; /* Block id of the first block group */
	int addrs_per_block; /* How many LBAs are in a block? */
	int indirectshift; /* shift for one level of indirection */
};

static inode* ext2_cache_alloc(context* context)
{
	/* acquire lock */
	slock_acquire(&context->cache_lock);
	inode* result = NULL;

	int x;
	for(x = 0;x < context->cache_count;x++)
	{
		if(!context->cache[x].allocated)
		{
			context->cache[x].allocated = 1;
			result = &context->cache[x];
			break;
		}
	}

	slock_release(&context->cache_lock);

	return result;
}

static void ext2_cache_free(inode* ino, context* context)
{
	slock_acquire(&context->cache_lock);
	/* Make sure there aren't any references left */
	if(!ino->references) memset(ino, 0, sizeof(inode));
	slock_release(&context->cache_lock);
}

static int ext2_calc_block_addr(int block_num, context* context)
{
	int addr = -1;
	
	// 0 1 (3 5 7)
	if(context->revision == 0)
	{
		/* Each block group has a superblock backup */
	} else if(context->revision == 1)
	{
		
	}

	return addr;
}

static int ext2_read_inode(disk_inode* dst, int num, context* context)
{
	char block[context->blocksize];
	if(num <= 0) return -1;
	int block_group = (num - 1) >> context->inodegroupshift;
	int local_index = (num - 1) & 
		(context->base_superblock.inodes_per_group - 1);

	cprintf("Block group: %d\n", block_group);
	cprintf("Local index: %d\n", local_index);

	/* TODO: block group calculation algorithm */
	int block_group_address = 2;
	ext2_calc_block_addr(block_group, context);

	/* Read the block group */
	if(context->driver->readblock(block, block_group_address, context->driver) != context->blocksize)
		return -1;
	struct ext2_block_group_table* table = (void*)block;

	/* Get the address of the inode */
	uint inode_offset = local_index << context->inodesizeshift;
	uint inode_address = table->inode_table + 
		(inode_offset >> context->blockshift);
	uint inode_block_offset = inode_offset & (context->blocksize - 1);

	/* Read from the inode table */
	context->driver->readblock(block, inode_address, context->driver);
	disk_inode* ino = (void*)(block + inode_block_offset);
	memmove(dst, ino, context->inodesize);

	return 0;
}

/**
 * Get the logical block address for a block in an inode.
 */
static int ext2_block_address(int index, disk_inode* ino, 
		context* context)
{
	char block[context->blocksize];
	int* i_block = (int*)block;
	/* Direct */
	if(index < EXT2_DIRECT_COUNT)
		return ino->direct[index];
	index -= EXT2_DIRECT_COUNT;

	/* Indirect */
	if(index < 1 << context->indirectshift)
	{
		context->driver->readblock(block, 
			ino->indirect, context->driver);
		return i_block[index];
	}

	index -= context->addrs_per_block;
	/* 2 levels of indirection */
	if(index < 1 << context->indirectshift << 
		context->indirectshift)
	{
		int upper = (index >> context->indirectshift)
			& (context->addrs_per_block - 1);
		int lower = index & (context->addrs_per_block - 1);

		context->driver->readblock(block,
                        ino->dindirect, context->driver);
		int indirect = i_block[upper];
		context->driver->readblock(block,
                        indirect, context->driver);
		return i_block[lower];
	}

	/* REALLY big files */
	index -= (1 << context->indirectshift << context->indirectshift);
	if(index < 1 << context->indirectshift 
		<< context->indirectshift
		<< context->indirectshift)
	{
		int upper = (index >> context->indirectshift
			>> context->indirectshift)
                        & (context->addrs_per_block - 1);
		int middle = (index >> context->indirectshift)
                        & (context->addrs_per_block - 1);
                int lower = index & (context->addrs_per_block - 1);

                context->driver->readblock(block,
                        ino->tindirect, context->driver);
                int dindirect= i_block[upper];
                context->driver->readblock(block,
                        dindirect, context->driver);
		int indirect= i_block[middle];
                context->driver->readblock(block,
                        indirect, context->driver);
                return i_block[lower];
	}

	return -1;
}

static int _ext2_read(void* dst, uint start, uint sz, 
		disk_inode* ino, context* context)
{
	void* dst_c = dst;
	uint bytes = 0;
	uint read = 0;
	char block[context->blocksize];
	int start_index = start >> context->blockshift;
	int end_index = (start + sz) >> context->blockshift;

	/* read the first block */
	int lba = ext2_block_address(start_index, ino, context);
	if(context->driver->readblock(block, lba, 
				context->driver) != context->blocksize)
		return -1;
	read = context->blocksize - (start & (context->blocksize - 1));
	if(read > sz) read = sz;
	memmove(dst, block + (start & (context->blocksize - 1)), read);
	if(read == sz) return sz;	

	/* Read middle blocks */
	int x = start_index + 1;
	for(;x < end_index;x++)
	{
		lba = ext2_block_address(x, ino, context);
		if(context->driver->readblock(block, lba, 
					context->driver) !=context->blocksize)
			return -1;
		memmove(dst_c + bytes, block, context->blocksize);
		bytes += context->blocksize;
	}

	/* Are we done? */
	if(bytes == sz) return sz;

	/* read final block */
	lba = ext2_block_address(x, ino, context);
	if(context->driver->readblock(block, lba, 
				context->driver) !=context->blocksize)
		return -1;
	memmove(dst_c + bytes, block, sz - bytes);

	return sz;
}

static int ext2_lookup_rec(const char* path, struct ext2_dirent* dst, 
		disk_inode* handle, context* context)
{
	uint_64 file_size = handle->lower_size | 
		((uint_64)handle->upper_size << 32);
	char parent[EXT2_MAX_PATH];
	struct ext2_dirent dir;
	uchar last = 0;
	if(!strcmp(parent, path)) last = 1;

	if(file_path_root(path, parent, EXT2_MAX_PATH)) return -1;
	/* Search for the parent in the handle */
	int pos = 0;
	for(pos = 0;pos < file_size;)
	{
		uint_16 size;
		if(_ext2_read(&size, 4, 2, handle, context) != 2)
			return -1;
		/* Now that we know the size, read the full dirent */
		if(_ext2_read(&dir, pos, size, handle, context) != size)
			return -1;
		if(!strcmp(dir.name, parent))
		{
			if(last)
			{
				if(dst) memmove(dst, &dir, size);
				return dir.inode;
			} else {
				path = file_remove_prefix(path);
				return ext2_lookup_rec(path, dst, 
						handle, context);
			}
		}
	}

	return 0;
}

static int ext2_lookup(const char* path, struct ext2_dirent* dst, 
		context* context)
{
	return ext2_lookup_rec(path, dst, &context->root->ino, context);
}

static int ext2_lookup_inode(const char* path, disk_inode* dst, 
                context* context)
{
	int ino = ext2_lookup(path, NULL, context);
	if(ext2_read_inode(dst, ino, context))
		return -1;
	return ino;
	
}

int ext2_test(context* context)
{
	disk_inode ino;
	int inonum = ext2_lookup_inode("/bin/bash", &ino, context);
	cprintf("Inode number for /bin/bash: %d\n", inonum);

	/* SUPPRESS WARNINGS */
	ext2_cache_free(ext2_cache_alloc(context), context);
	ext2_lookup(NULL, NULL, context);
	return 0;
}

int ext2_init(uint superblock_address, uint sectsize,
		uint cache_sz, struct FSHardwareDriver* driver,
		struct FSDriver* fs)
{
	if(sizeof(context) > FS_CONTEXT_SIZE)
	{
		cprintf("EXT2 Fatal error: not enough context space.\n");
		return -1;
	}

	fs->driver = driver;
	fs->valid = 1;
	fs->type = 0; /* TODO: assign a number to ext2 */
	memset(fs->context, FS_CONTEXT_SIZE, 0);
	memset(fs->cache, FS_CACHE_SIZE, 0);

	context* context = (void*)fs->context;
	slock_init(&context->cache_lock);
	context->cache_count = cache_sz / sizeof(context);
	context->cache = (void*)fs->cache;
	context->driver = driver;
	context->sectsize = sectsize;
	context->fs = fs;

	/* enable read and write for the driver */
	diskio_setup(driver);

	/* Read the superblock */
	char super_buffer[1024];
	if(driver->read(super_buffer, 1024, 1024, driver) != 1024)
		return -1;
	struct ext2_base_superblock* base = (void*) super_buffer;
	struct ext2_extended_base_superblock* extended_base =
		(void*)(super_buffer +
				sizeof(struct ext2_base_superblock));

	memmove(&context->base_superblock, base, 
			sizeof(struct ext2_base_superblock));
	if(base->major_version >= 1)
	{
		memmove(&context->extended_superblock, extended_base, 
				sizeof(struct ext2_extended_base_superblock));
	}

	/* Calculate the shift value */
	context->inodegroupshift = 
		log2(context->base_superblock.inodes_per_group);
	context->revision = base->major_version;
	context->sectsize = driver->sectsize;
	context->blocksize = 1024 << base->block_size_shift;
	context->blockshift = base->block_size_shift + 10;
	context->firstgroup = 1024 >> context->blockshift;
	context->addrs_per_block = context->blocksize >> 2;
	context->indirectshift = log2(context->addrs_per_block);
	if(context->indirectshift < 0) return -1;
	/* set the driver block size and shift */
	driver->blocksize = context->blocksize;
	driver->blockshift = context->blockshift;

	if(context->revision == 1)
	{
		/* inode size is variable */
		context->inodesize = extended_base->inode_size;
	} else if(context->revision == 0)
	{
		/* Inode size is fixed at 128 */
		context->inodesize = 128;	
	} else {
		printf("Invalid revision type.\n");
		return -1;
	}

	context->inodesizeshift = log2(context->inodesize);
	if(context->inodesizeshift < 0) return -1;

	/* Cache the root inode */
	inode* root = ext2_cache_alloc(context);
	if(ext2_read_inode(&root->ino, extended_base->first_inode, context))
		return -1;
	context->root = root;

	return 0;
}

inode* ext2_opened(const char* path, context* context)
{
	slock_acquire(&context->cache_lock);
	inode* ino = NULL;
	int x;
	for(x = 0;x < context->cache_count;x++)
	{
		if(!strcmp(context->cache[x].path, path))
		{
			ino = context->cache + x;
			break;
		}
	}

	/* Did we find anything? */
	if(ino) ino->references++;
	slock_release(&context->cache_lock);
	return ino;
}

inode* ext2_open(const char* path, context* context)
{
	/* Check to see if it is already opened */
	inode* ino = NULL;
	if((ino = ext2_opened(path, context)))
		return ino;

	/* The file is not already opened. */
	ino = ext2_cache_alloc(context);
	return NULL;
}

void ext2_close(inode* ino, context* context)
{

}

int ext2_stat(inode* ino, struct stat* dst, context* context)
{
	return -1;
}

int ext2_create(const char* path, mode_t permissions, 
		uid_t uid, gid_t gid, context* context)
{
	return -1;
}

int ext2_chown(inode* ino, uid_t uid, gid_t gid, context* context)
{
	return -1;
}

int ext2_chmod(inode* ino, mode_t permission, context* context)
{
	return -1;
}

int ext2_truncate(inode* ino, int sz, context* context)
{
	return -1;
}

int ext2_link(const char* file, const char* link, context* context)
{
	return -1;
}

int ext2_symlink(const char* file, const char* link, context* context)
{
	return -1;
}

int ext2_mkdir(const char* path, mode_t permission, 
		uid_t uid, gid_t gid, context* context)
{
	return -1;
}

int ext2_rmdir(const char* path, context* context)
{
	return -1;
}

int ext2_read(inode* ino, void* dst, uint start, uint sz, 
		context* context)
{
	return -1;
}

int ext2_write(inode* ino, void* src, uint start, uint sz,
		context* context)
{
	return -1;
}

int ext2_rename(const char* src, const char* dst, context* context)
{
	return -1;
}

int ext2_unlink(const char* file, context* context)
{
	return -1;
}

int ext2_readdir(inode* dir, int index, struct dirent* dst, 
		context* context)
{
	return -1;
}

int ext2_fsstat(struct fs_stat* dst, context* context)
{
	return -1;
}

