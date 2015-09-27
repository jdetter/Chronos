/**
 * Author: John Detter <john@detter.com>
 *
 * Driver for the EXT2 file system.
 */

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
	if(value == 0) return -1;
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
#include "cache.h"
#include "diskcache.h"

#define EXT2_DIRECT_COUNT 12

#define EXT2_MAX_PATH FILE_MAX_PATH
#define EXT2_MAX_NAME FILE_MAX_NAME
#define EXT2_MAX_PATH_SEGS 32

/* Define some very basic types */

/**
 * An offset into a file or a file size.
 */
typedef uint_64 file_offset;
typedef uint_32 blkid;

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
	struct ext2_disk_inode* ino;
	void* block; /* The block that contains this inode */
	uint_32 inode_num;
	uint_32 inode_group;
	char path[EXT2_MAX_PATH];
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
	char unused[14];
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

/* EXT2 file types */
#define EXT2_FILE_UNKNOWN 	0x0
#define EXT2_FILE_REG_FILE 	0x1
#define EXT2_FILE_DIR 		0x2
#define EXT2_FILE_CHRDEV 	0x3
#define EXT2_FILE_BLKDEV 	0x4
#define EXT2_FILE_FIFO 		0x5
#define EXT2_FILE_SOCK 		0x6
#define EXT2_FILE_SYMLINK 	0x7

typedef struct ext2_disk_inode disk_inode;
typedef struct ext2_cache_inode inode;
typedef struct ext2_context context;

struct ext2_context
{
	struct cache inode_cache;
	inode* root; /* The root inode */

	/* superblock information */
	struct ext2_base_superblock base_superblock;
	struct ext2_extended_base_superblock extended_superblock;
	/* The block containing the superblock */
	char* super_block;
	/* The offset in the block containing the superblock */
	uint super_offset;

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
	int grouptableaddr; /* in which block is the bgdt? */
	int grouptableshift;
	int blockspergroup; /* how many blocks are in a group? */
	int blockspergroupshift; /* Turn a block into a block id */
	int groupcount; /* How many block groups are there? */
	int inodeblocks; /* How many blocks are inode blocks? (grp)*/
	int firstgroupstart; /* The first block of the first group */
};

static int ext2_write_bgdt(int group,
		struct ext2_block_group_table* src, context* context)
{
	char* block;
	int block_group_addr = context->grouptableaddr;
	int offset = group << context->grouptableshift;
	block_group_addr += (offset >> context->blockshift);
	offset &= context->blocksize - 1;

	/* Read the block group */
	block = context->driver->reference(block_group_addr, context->driver);
	if(!block) return -1;
	struct ext2_block_group_table* table = (void*)(block + offset);
	memmove(table, src, sizeof(struct ext2_block_group_table));

	/* we're done with the block */
	context->driver->dereference(block, context->driver);

	return 0;
}

static int ext2_read_bgdt(int group, 
		struct ext2_block_group_table* dst, context* context)
{
	char* block;
	int block_group_addr = context->grouptableaddr;
	int offset = group << context->grouptableshift;
	block_group_addr += (offset >> context->blockshift);
	offset &= context->blocksize - 1;

	/* Read the block group */
	block = context->driver->reference(block_group_addr, context->driver);
	if(!block) return -1;
	struct ext2_block_group_table* table = (void*)(block + offset);
	memmove(dst, table, 1 << context->grouptableshift);
	/* dereference the block */
	context->driver->dereference(block, context->driver);

	return 0;
}

static int ext2_get_bit(char* block, int bit, context* context)
{
	int byte = bit >> 3; /* 8 bits in a byte */
	int bit_offset = bit & 7; /* Get last 3 bits */
	if(byte >= context->blocksize)
		return -1; /* Bit doesn't exist! */
	return (block[byte] >> bit_offset) & 0x1;
}

static int ext2_set_bit(char* block, int bit, int val, context* context)
{
	int byte = bit >> 3; /* 8 bits in a byte */
	int bit_offset = bit & 7; /* Get last 3 bits */
	if(byte >= context->blocksize)
		return -1; /* Bit doesn't exist! */
	if(val) block[byte] |= 1 << bit_offset;
	else block[byte] &= ~(1 << bit_offset);

	return 0;
}

static int ext2_alloc_inode(int num, int dir, context* context)
{
	char* block;
	if(num <= 0) return -1;
	int block_group = (num - 1) >> context->inodegroupshift;
	int local_index = (num - 1) &
		(context->base_superblock.inodes_per_group - 1);

	struct ext2_block_group_table table;
	if(ext2_read_bgdt(block_group, &table, context))
		return -1;

	block = context->driver->reference(table.inode_bitmap_address,
			context->driver);
	if(!block) return -1; /* cache failed */

	/* Make sure this inode is not already allocated */
	if(ext2_get_bit(block, local_index, context))
	{
		context->driver->dereference(block, context->driver);
		return -1; /* Already allocated */
	}
	/* Set the bit */
	if(ext2_set_bit(block, local_index, 1, context))
	{
		context->driver->dereference(block, context->driver);
		return -1;
	}

	/* Update table metadata */
	table.free_inodes--;
	if(dir) table.dir_count++;

	/* Update superblock */
	context->base_superblock.free_inode_count--;

	/* Write the updated table */
	if(ext2_write_bgdt(block_group, &table, context))
	{
		context->driver->dereference(block, context->driver);
		return -1;
	}

	context->driver->dereference(block, context->driver);
	return 0;
}

static int ext2_free_inode(int num, int dir, context* context)
{
	char* block;
	if(num <= 0) return -1;
	int block_group = (num - 1) >> context->inodegroupshift;
	int local_index = (num - 1) &
		(context->base_superblock.inodes_per_group - 1);

	struct ext2_block_group_table table;
	if(ext2_read_bgdt(block_group, &table, context))
		return -1;

	/* Read the bitmap */
	block = context->driver->reference(table.inode_bitmap_address,
			context->driver);
	if(!block) return -1;
	/* Make sure this inode is allocated */
	if(!ext2_get_bit(block, local_index, context))
	{
		context->driver->dereference(block, context->driver);
		return -1; /* not allocated */
	}
	/* Clear the bit */
	if(ext2_set_bit(block, local_index, 0, context))
	{
		context->driver->dereference(block, context->driver);
		return -1;
	}

	/* Update table metadata */
	table.free_inodes++;
	if(dir) table.dir_count--;

	/* Update superblock */
	context->base_superblock.free_inode_count++;

	/* Write the updated table */
	if(ext2_write_bgdt(block_group, &table, context))
	{
		context->driver->dereference(block, context->driver);
		return -1;
	}

	context->driver->dereference(block, context->driver);
	return 0;
}

static int ext2_find_free_inode_rec(int group, int dir, context* context)
{
	char* block;

	/* Get the table */
	struct ext2_block_group_table table;
	if(ext2_read_bgdt(group, &table, context))
		return -1;

	/* Read the bitmap */
	block = context->driver->reference(table.inode_bitmap_address,
			context->driver);
	if(!block) return -1;

	int x = 0;
	if(group == 0)
		x = context->extended_superblock.first_inode;
	int found = 0;
	for(;x < context->base_superblock.inodes_per_group;x++)
	{
		if(!ext2_get_bit(block, x, context))
		{
			/* Found a free inode */
			found = 1;
			break;
		}
	}

	if(!found) 
	{
		context->driver->dereference(block, context->driver);
		return -1;
	}

	int ino_num = x + (group << context->inodegroupshift) + 1;

	/* allocate this inode */
	if(ext2_alloc_inode(ino_num, dir, context))
	{
		context->driver->dereference(block, context->driver);
		return -1; /* Couldn't be allocated? */
	}

	context->driver->dereference(block, context->driver);

	return ino_num;
}

static int ext2_find_free_inode(int hint, int dir, context* context)
{
	/* First try the hinted group */
	int result = ext2_find_free_inode_rec(hint, dir, context);
	if(result >= 0) return result;

	int x;
	for(x = 0;x < context->groupcount;x++)
	{
		if(x == hint) continue; /* Dont retry hint */
		result = ext2_find_free_inode_rec(x, dir, context);
		if(result >= 0) return result;
	}

	return -1;
}

static int ext2_alloc_block(int block_group, int blockid, context* context)
{
	char* block;
	/* Get the block group descriptor table */
	struct ext2_block_group_table table;
	if(ext2_read_bgdt(block_group, &table, context))
		return -1;
	/* Dont allow blocks to be allocated that point to metadata */
	if(blockid < table.inode_table + context->inodeblocks)
		return -1;

	/* Is the block free? */
	block = context->driver->reference(table.block_bitmap_address, 
			context->driver);
	if(!block) return -1;

	/* Is the block already allocated? */
	if(ext2_get_bit(block, blockid, context))
	{
		context->driver->dereference(block, context->driver);
		return -1; /* Already allocated!*/
	}

	/* Set the bit */
	if(ext2_set_bit(block, blockid, 1, context))
	{
		context->driver->dereference(block, context->driver);
		return -1;
	}

	/* update block group table metadata (free blocks - 1)*/
	table.free_blocks--;
	if(ext2_write_bgdt(block_group, &table, context))
	{
		context->driver->dereference(block, context->driver);
		return -1;
	}

	/* Update the superblock */
	context->base_superblock.free_block_count--;

	context->driver->dereference(block, context->driver);

	return 0;
}

static int ext2_free_block(int block_num, context* context)
{
	char* block;
	struct ext2_block_group_table table;

	/* Lets find the group that this block belongs to */
	int group = (block_num - context->firstgroupstart) 
		>> context->blockspergroupshift;
	if(ext2_read_bgdt(group, &table, context)) return -1;
	int blockid = block_num & (context->blockspergroup - 1);
	/* Dont allow blocks to be freed that point to metadata */
	if(blockid < table.inode_table + context->inodeblocks)
		return -1;

	/* Load the bitmap table */
	block = context->driver->reference(table.block_bitmap_address, 
			context->driver);
	if(!block) return -1;

	/* Is the block allocated? */
	if(!ext2_get_bit(block, blockid, context))
	{
		context->driver->dereference(block, context->driver);
		return -1; /* Already free */
	}

	/* Unset the bit */
	if(ext2_set_bit(block, blockid, 0, context))
	{
		context->driver->dereference(block, context->driver);
		return -1;
	}

	/* update block group table metadata (free blocks - 1)*/
	table.free_blocks++;
	if(ext2_write_bgdt(group, &table, context))
	{
		context->driver->dereference(block, context->driver);
		return -1;
	}

	/* Update the superblock */
	context->base_superblock.free_block_count++;

	context->driver->dereference(block, context->driver);

	return 0;
}

static int ext2_find_free_blocks_rec(int group, int contiguous, 
		context* context)
{
	struct ext2_block_group_table table;
	int group_start = (group << context->blockspergroup) 
		+ context->firstgroupstart;
	char* block;

	/* Read the table descriptor */
	if(ext2_read_bgdt(group, &table, context))
		return -1;

	block = context->driver->reference(table.block_bitmap_address,
			context->driver);
	if(!block) return -1;

	int sequence = 0; /* How many free in a row have we found? */
	int range_start = -1;

	/* We are starting the search just after the end of the metadata. */
	int x = (table.inode_table + context->inodeblocks) 
		& (context->blockspergroup - 1);
	for(;x < context->blockspergroup;x++)
	{
		/* Are there any free blocks in this byte? */
		if(!(x & 7) /* Is this a new byte? */
				&& block[x & ~7] == 0xFF)
		{
			/* This byte is completely allocated */
			x += 8;
			sequence = 0; /* reset the sequence */
			continue;
		}

		/* Check this bit */
		if(ext2_get_bit(block, x, context))
		{
			/* Allocated! */
			sequence = 0;
			range_start = -1;
		} else {
			sequence++;
			if(sequence == 1)
				range_start = x;

			if(sequence == contiguous)
			{
				/* Were done! */
				break;
			}
		}
	}

	/* Could we find enough blocks? */
	if(sequence != contiguous) 
	{
		context->driver->dereference(block, context->driver);
		return -1;
	}

	/* Allocate the range */
	for(x = 0;x < sequence;x++)
	{
		if(ext2_alloc_block(group, range_start + x, context))
		{
			context->driver->dereference(block, context->driver);
			return -1;
		}
	}


	context->driver->dereference(block, context->driver);

	return range_start + group_start;
}

static int ext2_find_free_blocks(int hint,
		int contiguous, context* context)
{
	int result;
	if(hint >= 0)
	{
		/* Attempt the hint first */
		result = ext2_find_free_blocks_rec(hint, contiguous, context);
		if(result > 0) return result;
	}

	/* Try every group except the hint */
	int x;
	for(x = 0;x < context->groupcount;x++)
	{
		if(x == hint) continue;

		result = ext2_find_free_blocks_rec(hint, contiguous, context);
		if(result > 0) return result;
	}

	/** 
	 * The disk is full, or the disk is too fragmented or the requested
	 * amount of contiguous blocks is too large.
	 */
	return -1;
}

static int _ext2_read_inode(inode* dst, int num, context* context)
{
	char* block;
	if(num <= 0) return -1;
	int block_group = (num - 1) 
		>> context->inodegroupshift;
	int local_index = (num - 1) & 
		(context->base_superblock.inodes_per_group - 1);

	struct ext2_block_group_table table;
	ext2_read_bgdt(block_group, &table, context);

	/* Get the address of the inode */
	uint inode_offset = local_index << context->inodesizeshift;
	uint inode_address = table.inode_table + 
		(inode_offset >> context->blockshift);
	uint inode_block_offset = inode_offset & (context->blocksize - 1);

	/* Read from the inode table */
	block = context->driver->reference(inode_address, context->driver);
	if(!block) return -1;
	dst->block = block;
	dst->ino = (void*)(block + inode_block_offset);

	return 0;
}

static int ext2_inode_cache_populate(void* obj, int id, void* c)
{
	context* context = c;
	inode* ino = obj;
	ino->inode_num = id;
	ino->inode_group = id >> context->inodegroupshift;
	/* Can't do path from here */
	memset(ino->path, 0, EXT2_MAX_PATH);

	return _ext2_read_inode(obj, id, c);
}

static int ext2_inode_cache_eject(void* obj, int id, void* c)
{
	context* context = c;
	inode* ino = obj;
	if(!ino || !context) return -1;

	context->driver->dereference(ino->block, context->driver);
	return 0;
}

static int ext2_inode_cache_sync(void* obj, int id, struct cache* cache)
{
	/* Inodes are automatically synced on block buffer sync */
	return 0;
}	

static int ext2_inode_cache_query(void* query_obj, void* test_obj, void* c)
{
	inode* query = query_obj;
	inode* test = test_obj;

	/* If the query object has no path, we can't do anything. */
	if(strlen(query->path) == 0) return -1;

	if(!strcmp(query->path, test->path))
		return 0;
	return -1;	
}

/**
 * Get the logical block address for a block in an inode.
 */
static int ext2_block_address(int index, disk_inode* ino, 
		context* context)
{
	int address;
	int* i_block;
	/* Direct */
	if(index < EXT2_DIRECT_COUNT)
		return ino->direct[index];
	index -= EXT2_DIRECT_COUNT;

	/* Indirect */
	if(index < 1 << context->indirectshift)
	{
		i_block = context->driver->reference(ino->indirect, 
				context->driver);
		if(!i_block) return -1;
		address = i_block[index];
		context->driver->dereference(i_block, context->driver);
		return address;
	}

	index -= context->addrs_per_block;
	/* 2 levels of indirection */
	if(index < 1 << context->indirectshift << 
			context->indirectshift)
	{
		int upper = (index >> context->indirectshift)
			& (context->addrs_per_block - 1);
		int lower = index & (context->addrs_per_block - 1);

		i_block = context->driver->reference(ino->dindirect, 
				context->driver);
		if(!i_block) return -1;
		int indirect = i_block[upper];
		context->driver->dereference(i_block, context->driver);
		i_block = context->driver->reference(indirect, 
				context->driver);
		if(!i_block) return -1;
		address = i_block[lower];
		context->driver->dereference(i_block, context->driver);
		return address;
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

		i_block = context->driver->reference(ino->tindirect,
				context->driver);
		if(!i_block) return -1;
		int dindirect = i_block[upper];
		context->driver->dereference(i_block, context->driver);
		i_block = context->driver->reference(dindirect,
				context->driver);
		if(!i_block) return -1;
		int indirect = i_block[middle];
		context->driver->dereference(i_block, context->driver);
		i_block = context->driver->reference(indirect,
				context->driver);
		address = i_block[lower];
		context->driver->dereference(i_block, context->driver);
		return address;
	}

	return -1;
}

/**
 * Set the logical block address for a block in an inode.
 * TODO: there are a lot of performance improvements that can be made here.
 */
static int ext2_set_block_address(int index, int val, int block_hint,
		disk_inode* ino, context* context)
{
	int* i_block;
	/* Direct */
	if(index < EXT2_DIRECT_COUNT)
	{
		ino->direct[index] = val;
		/* No write needed here, inodes are cached */
		return 0;
	}
	index -= EXT2_DIRECT_COUNT;

	/* Indirect */
	if(index < 1 << context->indirectshift)
	{
		int indirect = ino->indirect;
		/* If there is no indirect block, were making one. */
		if(!indirect)
		{
			/* Find any free block */
			indirect = ext2_find_free_blocks(block_hint, 1,
					context);
			if(indirect < 0)
				return -1;
			/* update inode */
			ino->indirect = indirect;
			i_block = context->driver->addreference(indirect, 
					context->driver);
			if(!i_block) return -1;

			/* zero the block*/
			memset(i_block, 0, context->blocksize);
		} else {
			i_block = context->driver->reference(indirect,
					context->driver);
			if(!i_block) return -1;
		}

		i_block[index] = val;
		context->driver->dereference(i_block, context->driver);
		return 0;
	}

	index -= context->addrs_per_block;
	/* 2 levels of indirection */
	if(index < 1 << context->indirectshift <<
			context->indirectshift)
	{
		/* Do we have a double indirect block? */
		int dindirect = ino->dindirect;
		if(!dindirect)
		{
			/* Find any free block */
			dindirect = ext2_find_free_blocks(block_hint, 1,
					context);
			if(dindirect < 0)
				return -1;

			/* update inode pointer */
			ino->dindirect = dindirect;
			i_block = context->driver->addreference(dindirect,
					context->driver);
			if(!i_block) return -1;
			/* zero the block*/
			memset(i_block, 0, context->blocksize);
		} else {
			i_block = context->driver->reference(dindirect,
					context->driver);
			if(!i_block) return -1;
		}

		int upper = (index >> context->indirectshift)
			& (context->addrs_per_block - 1);
		int lower = index & (context->addrs_per_block - 1);

		int indirect = i_block[upper];

		/* Do we have an indirect? */
		if(indirect == 0)
		{
			/* Find any free block */
			indirect = ext2_find_free_blocks(block_hint, 1,
					context);
			if(indirect < 0)
			{
				context->driver->dereference(i_block, 
						context->driver);
				return -1;
			}
			/* update the block*/
			i_block[upper] = indirect;
			context->driver->dereference(i_block, context->driver);

			/* get new cache block */
			i_block = context->driver->addreference(indirect,
					context->driver);
			if(!i_block) return -1;

			/* zero the block*/
			memset(i_block, 0, context->blocksize);
		} else {
			context->driver->dereference(i_block, context->driver);
			i_block = context->driver->reference(indirect,
					context->driver);
			if(!i_block) return -1;
		}

		i_block[lower] = val;
		/* update indirect */
		context->driver->dereference(i_block, context->driver);
		return 0;

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

		int tindirect = ino->tindirect;
		/* Do we have a triple indirect? */
		if(!tindirect)
		{
			/* Find any free block */
			tindirect = ext2_find_free_blocks(block_hint, 1,
					context);
			if(tindirect < 0)
				return -1;

			/* update inode */
			ino->tindirect = tindirect;

			i_block = context->driver->addreference(tindirect,
					context->driver);
			if(!i_block) return -1;

			/* zero the block*/
			memset(i_block, 0, context->blocksize);
		} else {
			i_block = context->driver->reference(tindirect,
					context->driver);
			if(!i_block) return -1;
		}

		int dindirect = i_block[upper];
		/* Do we have a dindirect? */
		if(!dindirect)
		{
			/* Find any free block */
			dindirect = ext2_find_free_blocks(block_hint, 1,
					context);
			if(dindirect < 0)
			{
				context->driver->dereference(i_block, 
						context->driver);
				return -1;
			}

			i_block[upper] = dindirect;
			context->driver->dereference(i_block, context->driver);
			i_block = context->driver->addreference(dindirect,
					context->driver);
			if(!i_block) return -1;
			memset(i_block, 0, context->blocksize);
		} else {
			context->driver->dereference(i_block, context->driver);
			i_block = context->driver->reference(dindirect,
					context->driver);
			if(!i_block) return -1;
		}

		int indirect = i_block[middle];

		/* Do we have an indirect? */
		if(!indirect)
		{
			/* Find any free block */
			indirect = ext2_find_free_blocks(block_hint, 1,
					context);
			if(indirect < 0)
			{
				/* No more free blocks */
				context->driver->dereference(i_block, 
						context->driver);
				return -1;
			}

			/* zero the block*/
			i_block[middle] = indirect;
			context->driver->dereference(i_block, context->driver);
			i_block = context->driver->addreference(indirect,
					context->driver);
			if(!i_block) return -1;
			memset(i_block, 0, context->blocksize);
		} else {
			context->driver->dereference(i_block, context->driver);
			i_block = context->driver->reference(indirect,
					context->driver);
			if(!i_block) return -1;
		}

		i_block[lower] = val;
		/* write back the block */
		context->driver->dereference(i_block, context->driver);

		return 0;
	}

	return -1;
}

static int _ext2_read(void* dst, uint start, uint sz, 
		disk_inode* ino, context* context)
{
	uint_64 file_size = ino->lower_size |
		((uint_64)ino->upper_size << 32);

	if(start >= file_size) return 0; /* End of file */
	/* Don't allow reads past the end of the file. */
	if(start + sz > file_size)
		sz = file_size - start;

	char* dst_c = dst;
	uint bytes = 0;
	uint read = 0;
	char* block;
	int start_index = start >> context->blockshift;
	int end_index = (start + sz) >> context->blockshift;

	/* read the first block */
	int lba = ext2_block_address(start_index, ino, context);
	block = context->driver->reference(lba, context->driver);
	if(!block) return -1;

	read = context->blocksize - (start & (context->blocksize - 1));
	if(read > sz) read = sz;
	memmove(dst, block + (start & (context->blocksize - 1)), read);
	context->driver->dereference(block, context->driver);
	if(read == sz) return sz;
	bytes += read;

	/* Read middle blocks */
	int x = start_index + 1;
	for(;x < end_index;x++)
	{
		lba = ext2_block_address(x, ino, context);
		block = context->driver->reference(lba, context->driver);
		if(!block) return -1;
		memmove(dst_c + bytes, block, context->blocksize);
		context->driver->dereference(block, context->driver);
		bytes += context->blocksize;
	}

	/* Are we done? */
	if(bytes == sz) return sz;

	/* read final block */
	lba = ext2_block_address(x, ino, context);
	block = context->driver->reference(lba, context->driver);
	if(!block) return -1;
	memmove(dst_c + bytes, block, sz - bytes);
	context->driver->dereference(block, context->driver);

	return sz;
}

/**
 * The writing algorithm is much more complex here. I have optimized it to
 * handle long writes so that the file will have long contiguous sequences
 * of blocks. This will make sequential writes very very fast. A huge
 * performance gain here is to ask: when should we replace blocks? When
 * they cause a fragment? When they are on a 'seam'?
 */
static int _ext2_write(const void* src, uint start, uint sz, 
		int group_hint, disk_inode* ino, context* context)
{
	char* block;

	/* 
	 * If we're writing past the end of the file, we 
	 * need to add blocks where were writing
	 */
	uint_64 file_size = ino->lower_size |
		((uint_64)ino->upper_size << 32);
	uint_64 start_64 = start;
	uint_64 sz_64 = sz;
	uint_64 end_write = start_64 + sz_64;

	/* Are we growing the file? */
	if(end_write > file_size)
	{
		/* Update size */
		ino->lower_size = (uint_32)end_write;
		ino->upper_size = (uint_32)(end_write >> 32);
	}

	/* We need to add blocks where were about to write */

	int start_alloc = start >> context->blockshift;
	int end_alloc = (start + sz) >> context->blockshift;

	/* Ask for a contiguous sequence of blocks in our group */
	int sequence_start = ext2_find_free_blocks(group_hint, 
			end_alloc - start_alloc + 1, context);
	if(sequence_start < 0) return -1;

	int x = 0;
	for(;x + start_alloc < end_alloc + 1;x++)
	{
		/* Get the lba of the existing block */
		int lba = ext2_block_address(start_alloc + x, ino, context);

		if(lba && (x == 0 || x + start_alloc == end_alloc))
		{
			/* Contents should be copied */

			/* Read data from old location */
			char* old_location = context->driver->reference(lba,
					context->driver);
			if(!old_location) return -1;
			char* new_location = context->driver->addreference(
					sequence_start + x,
					context->driver);
			if(!new_location)
			{
				context->driver->dereference(old_location,
						context->driver);
				return -1;
			}

			/* Copy to new location */
			memmove(new_location, old_location, context->blocksize);
			context->driver->dereference(new_location, 
					context->driver);
			context->driver->dereference(old_location,
					context->driver);
		}

		/* Install new lba */
		if(ext2_set_block_address(x + start_alloc, sequence_start + x,
					group_hint, ino, context))
			return -1;

		/* Free old lba if there was one */
		if(lba) ext2_free_block(lba, context);
	}

	const char* src_c = src;
	uint bytes = 0;
	uint write = 0;
	int start_index = start >> context->blockshift;
	int end_index = (start + sz) >> context->blockshift;

	/* read the first block */
	write = context->blocksize - (start & (context->blocksize - 1));
	if(write > sz) write = sz;
	int lba = ext2_block_address(start_index, ino, context);

	if(write != context->blocksize)
	{
		block = context->driver->reference(lba, context->driver);
		if(!block) return -1;
	} else {
		block = context->driver->addreference(lba, context->driver);
		if(!block) return -1;
	}

	memmove(block + (start & (context->blocksize - 1)), src, write);
	/* Now we can write the block back to disk */
	context->driver->dereference(block, context->driver);
	if(write == sz) return sz;
	bytes += write;

	/* write middle blocks */
	x = start_index + 1;
	for(;x < end_index;x++)
	{
		lba = ext2_block_address(x, ino, context);
		block = context->driver->addreference(lba, context->driver);
		if(!block) return -1;
		memmove(block, src_c + bytes, context->blocksize);
		context->driver->dereference(block, context->driver);
		bytes += context->blocksize;
	}

	/* Are we done? */
	if(bytes == sz) return sz;
	lba = ext2_block_address(x, ino, context);

	/* read final block */
	if(sz - bytes == context->blocksize)
	{
		block = context->driver->reference(lba, context->driver);
		if(!block) return -1;
	} else {
		block = context->driver->addreference(lba, context->driver);
		if(!block) return -1;
	}

	memmove(block, src_c + bytes, sz - bytes);
	context->driver->dereference(block, context->driver);

	/* The inode will get flushed when it is written to disk. */

	return sz;
}

/**
 * Read a directory entry. Returns 0 on success.
 */
static int _ext2_readdir(struct ext2_dirent* dst, int pos, 
		disk_inode* ino, context* context)
{
	/**
	 * The metadata of a directory entry is as follows:
	 *
	 * inode number  | 4 bytes
	 * size of entry | 2 bytes
	 * name length   | 1 byte
	 * type of file  | 1 byte
	 * name          | name length bytes
	 * total possible length: 8 + EXT2_MAX_NAME
	 */

	if(pos < 0) return -1;

	uchar name_length;
	/* Get the size of the record */
	if(_ext2_read(&name_length, pos + 6, 1, ino, context) != 1)
		return -1;
	int readlen = name_length + 8;
	if(readlen > sizeof(struct ext2_dirent))
		readlen = sizeof(struct ext2_dirent);
	/* Now that we know the name length, read the full dirent */
	memset(dst, 0, sizeof(struct ext2_dirent));
	if(_ext2_read(dst, pos, readlen, ino, context) != readlen)
		return -1;

	return 0;
}

static int ext2_modify_dirent_type(disk_inode* dir, char* name, int group,
	uint_8 type, context* context)
{
	 if(!dir || !name) return -1;

        uint_64 dir_size = dir->lower_size |
                ((uint_64)dir->upper_size << 32);

        uint pos = 0;
        struct ext2_dirent current;

        while(pos < dir_size)
        {
                _ext2_readdir(&current, pos, dir, context);

		if(!strcmp(current.name, name))
		{
			current.type = type;
			if(_ext2_write(&current, pos, 8, group,
				dir, context) != 8) return -1;
			return 0;
		}
                
		/* Keep searching. */
                pos += current.size;
        }

	return -1;
}

/* Returns the inode number of the newly created directory entry */
/* Round a number up to the 4th bit boundary */
#define EXT2_ROUND_B4_UP(num) (((num) + 3) & ~3)
static int ext2_alloc_dirent(disk_inode* dir, int inode_num, 
		int group, const char* file, uchar type, 
		context* context)
{
	if(inode_num < 0) return -1;

	uint_64 dir_size = dir->lower_size |
		((uint_64)dir->upper_size << 32);

	/* with utf8 encoding can be no longer than 255 bytes. */
	if(strlen(file) > 255) return -1;

	/* How much space do we need? */
	int needed = EXT2_ROUND_B4_UP(8 + strlen(file));
	/* This is the position of the dirent we are amending */
	/* OR if not amending, the position of the new dirent */
	uint pos = 0;

	uint found = 0; /* Did we find a match? */

	struct ext2_dirent current;

	int available;
	while(pos < dir_size)
	{
		_ext2_readdir(&current, pos, dir, context);

		/* Check for match */
		available = current.size
			- EXT2_ROUND_B4_UP((8 + current.name_length));

		if(available >= needed)
		{
			found = 1;
			break;
		}

		/* Keep searching. */
		pos += current.size;
	}

	if(!found)
	{
		/* We couldn't find a suitable match. */
		/* Create a brand new entry on a new block. */
		current.size = context->blocksize;
		current.inode = inode_num;
		current.name_length = strlen(file);
		current.type = type;
		memmove(current.name, file, strlen(file));

		/* Flush to disk */
		if(_ext2_write(&current, pos, 8 + strlen(file), group,
					dir, context) != 8 + strlen(file)) return -1;

		/* update the directory size */
		dir_size += context->blocksize;
		dir->lower_size = (uint_32)dir_size;
		dir->upper_size = (uint_32)(dir_size >> 32);
	} else {
		/* Allocate the size we need */
		current.size = EXT2_ROUND_B4_UP(current.name_length) + 8;
		/* Flush to disk */
		if(_ext2_write(&current, pos, 8, group, dir, context)
				!= 8) return -1;

		/* update to our new position */
		pos += current.size;
		/* make the new entry */
		current.size = available;
		current.inode = inode_num;
		current.name_length = strlen(file);
		current.type = type;
		memmove(current.name, file, strlen(file));

		/* Flush to disk */
		if(_ext2_write(&current, pos, 8 + strlen(file), 
					group, dir, context) 
				!= 8 + strlen(file))
			return -1;

		/* The directory size has not changed. */
	}

	return 0;
}

static int _ext2_truncate(disk_inode* ino, uint_64 size, context* context)
{
	/* Convert the size into a block address */
	uint last_index = (size + context->blocksize - 1) 
		>> context->blockshift;

	/* Slow method. */
	blkid block;

	for(;(block = ext2_block_address(last_index, ino, context));
			last_index++)
	{
		if(ext2_free_block(block, context))
			return -1;
		if(ext2_set_block_address(last_index, 0, 0, ino, context))
			return -1;
	}

	ino->lower_size = size;
	ino->upper_size = (size >> 32);

	return 0;
}

/* This isn't used anywhere else so lets undefine it */
#undef EXT2_ROUND_B4_UP

static int ext2_free_dirent(disk_inode* dir, int group,
		const char* file, context* context)
{
	uint_64 dir_size = dir->lower_size |
		((uint_64)dir->upper_size << 32);

	/* Lets find the directory entry */
	int found = 0;
	int curr_pos = 0;
	int last_pos = -1;
	struct ext2_dirent previous;
	struct ext2_dirent current;
	/* The next entry OR the previous entry is useful, not both. */
	struct ext2_dirent* next = &previous;

	while(curr_pos < dir_size)
	{
		_ext2_readdir(&current, curr_pos, dir, context);

		/* did we find it? */
		if(!strcmp(file, current.name))
		{
			found = 1;
			break;
		}

		/* Keep searching. */	
		last_pos = curr_pos;
		curr_pos += current.size;
		memmove(&previous, &current, 8);
	}

	if(!found) return -1;

	if(curr_pos == 0 || curr_pos == 12)
	{
		/* Dont remove the first or second entry */
		return -1;
	}

	/* See if they are on the same block */
	int curr_block = (curr_pos >> context->blockshift);
	int last_block = (last_pos >> context->blockshift);

	if(curr_block == last_block)
	{
		/* This is very simple, just add the sizes */
		previous.size += current.size;
		if(_ext2_write(&previous, last_pos, 8, group, dir, 
					context) != 8)
			return -1;
	} else {
		/* If its the last block, we should free it. */
		if(current.size >= context->blocksize)
		{
			if(curr_pos + context->blocksize == dir_size)
			{
				/* We should truncate the file */
				_ext2_truncate(dir, dir_size 
						- context->blocksize, context);
			} else {
				/* All we can do is make a null entry */
				current.name_length = 0;
				current.inode = 0;
				current.type = 0;
			}
		} else {
			_ext2_readdir(next, curr_pos + current.size, 
					dir, context);

			current.name_length = next->name_length;
			current.type = next->type;
			current.inode = next->inode;
			memmove(current.name, next->name, current.name_length);

			if(_ext2_write(&current, curr_pos, 
						8 + strlen(current.name),
						group, dir, context) 
					!= 8 + strlen(current.name)) 
				return -1;
		}
	}

	return 0;
}

static int ext2_lookup_rec(const char* path, struct ext2_dirent* dst,
		int follow, disk_inode* handle, context* context)
{
	uint_64 file_size = handle->lower_size | 
		((uint_64)handle->upper_size << 32);
	char parent[EXT2_MAX_PATH];
	struct ext2_dirent dir;

	/* Check if this is root */
	if(!strcmp("/", path))
	{
		if(dst) memset(dst, 0, sizeof(struct ext2_dirent));
		return context->root->inode_num;
	}

	/* First, kill prefix slashes */
	while(*path == '/')path++;
	strncpy(parent, path, EXT2_MAX_PATH);
	if(file_path_root(parent)) return -1;
	uchar last = 0;
	if(!strcmp(parent, path)) last = 1;
	/* Search for the parent in the handle */
	int pos = 0;
	for(pos = 0;pos < file_size;)
	{
		if(_ext2_readdir(&dir, pos, handle, context))
			return -1;
		if(!strcmp(dir.name, parent))
		{
			if(last)
			{
				if(dst) memmove(dst, &dir, 
						sizeof(struct ext2_dirent));
				return dir.inode;
			} else {
				path = file_remove_prefix(path);
				/* We have to read this inode */
				inode* new_handle = cache_reference(dir.inode, 
						&context->inode_cache);
				int result = -1;

				if(S_ISDIR(new_handle->ino->mode))	
					result = ext2_lookup_rec(path, dst, 
							follow, 
							new_handle->ino,
							context);
				else if(S_ISLNK(new_handle->ino->mode))
				{
					/* Start new search at root */

					/* TODO: ask kernel to resolve this */
				}

				cache_dereference(new_handle, 
						&context->inode_cache);
				return result;
			}
		} 	

		pos += dir.size;
	}

	return -1; /* didn't find anything! */
}

static int ext2_lookup(const char* path, struct ext2_dirent* dst, 
		context* context)
{
	return ext2_lookup_rec(path, dst, 1, context->root->ino, context);
}

inode* ext2_opened(const char* path, context* context);
inode* ext2_open(const char* path, context* context);
int ext2_close(inode* ino, context* context);
int ext2_stat(inode* ino, struct stat* dst, context* context);
int ext2_create(const char* path, mode_t permissions,
		uid_t uid, gid_t gid, context* context);
int ext2_chown(inode* ino, uid_t uid, gid_t gid, context* context);
int ext2_chmod(inode* ino, mode_t permission, context* context);
int ext2_truncate(inode* ino, int sz, context* context);
int ext2_link(const char* file, const char* link, context* context);
int ext2_symlink(const char* file, const char* link,context* context);
int ext2_mkdir(const char* path, mode_t permission,
		uid_t uid, gid_t gid, context* context);
int ext2_rmdir(const char* path, context* context);
int ext2_read(inode* ino, void* dst, uint start, uint sz,
		context* context);
int ext2_write(inode* ino, const void* src, uint start, uint sz,
		context* context);
int ext2_rename(const char* src, const char* dst, context* context);
int ext2_unlink(const char* file, context* context);
int ext2_readdir(inode* dir, int index, struct dirent* dst,
		context* context);
int ext2_fsstat(struct fs_stat* dst, context* context);
int ext2_getdents(inode* dir, struct dirent* dst_arr, uint count,
		uint pos, context* context);
void ext2_sync(context* context);
int ext2_fsync(inode* ino, context* context);

int ext2_test(context* context)
{
	ext2_sync(context);
	return 0;
}

int ext2_init(uint superblock_address, uint sectsize,
		uint cache_sz, struct FSDriver* fs)
{
	if(sizeof(context) > FS_CONTEXT_SIZE)
	{
		cprintf("ext2: not enough context space.\n");
		return -1;
	}

	struct FSHardwareDriver* driver = fs->driver;
	fs->valid = 1;
	fs->type = 0; /* TODO: assign a number to ext2 */
	memset(fs->context, 0, FS_CONTEXT_SIZE);
	memset(fs->cache, 0, fs->cache_sz);

	context* context = (void*)fs->context;
	context->driver = driver;
	context->fs = fs;
	context->sectsize = sectsize;

	/* enable read and write for the driver */
	diskio_setup(context->driver);

	/* Read the superblock */
	uint superblock_start = (superblock_address * sectsize) + 1024;
	char super_buffer[1024];
	if(driver->read(super_buffer, superblock_start, 
			1024, driver) != 1024)
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
	context->blockspergroup = base->blocks_per_group;
	context->firstgroupstart = base->superblock_address;
	context->blockspergroupshift = log2(context->blockspergroup);
	if(context->blockspergroupshift < 0)
	{
		cprintf("ext2: bad blocks per group value!\n");
		return -1;
	}
	context->groupcount = base->block_count / 
		base->blocks_per_group;
	context->grouptableshift = 5; /* size = 32*/
	if(sizeof(struct ext2_block_group_table) 
			!= (1 << context->grouptableshift))
	{
		cprintf("ext2: Bad block group table size!\n");
		return -1;
	}
	context->grouptableaddr = 1; /* 2048, 4096, 8192, ...*/
	if(context->blocksize == 512 )
		context->grouptableaddr = 4;
	else if(context->blocksize == 1024)
		context->grouptableaddr = 2;
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
		cprintf("ext2: Invalid revision type.\n");
		return -1;
	}

	context->inodesizeshift = log2(context->inodesize);
	if(context->inodesizeshift < 0) return -1;
	context->inodeblocks = (base->inodes_per_group << 
			context->inodesizeshift) >> 
		context->blockshift;

	/* Setup caches */
	uint inode_cache_sz = cache_calc_size(FS_MAX_INODE_CACHE, 
		sizeof(struct ext2_cache_inode));
	if(inode_cache_sz > cache_sz) return -1;
	uint inode_cache_start = (uint)fs->cache + cache_sz - inode_cache_sz;
	cache_init((void*)inode_cache_start, inode_cache_sz, sizeof(inode),
			context, "inode", &context->inode_cache);
	context->inode_cache.sync = ext2_inode_cache_sync;
	context->inode_cache.populate = ext2_inode_cache_populate;
	context->inode_cache.query = ext2_inode_cache_query;
	context->inode_cache.eject = ext2_inode_cache_eject;

	uint disk_cache_sz = cache_sz - inode_cache_sz;
	uint disk_cache_start = (uint)fs->cache;
	cache_init((void*)disk_cache_start, disk_cache_sz, context->blocksize,
			driver, "disk", &driver->cache);
	disk_cache_init(driver);

	/* Setup the superblock context pointer */
        uint superblock_block = 0;
	context->super_offset = 0;
        if(context->blocksize == 512)
                superblock_block = 2;
        else if(context->blocksize == 1024)
		superblock_block = 1;
	else context->super_offset = 1024;

	context->super_block = context->driver->reference(superblock_block, 
			context->driver);

	/* Cache the root inode */
	inode* root = cache_reference(2, &context->inode_cache);
	if(!root) return -1;
	context->root = root;

	fs->open = (void*)ext2_open;
	fs->readdir = (void*)ext2_readdir;
	fs->close = (void*)ext2_close;
	fs->chmod = (void*)ext2_chmod;
	fs->chown = (void*)ext2_chown;
	fs->stat = (void*)ext2_stat;
	fs->read = (void*)ext2_read;
	fs->getdents = (void*)ext2_getdents;
	fs->unlink = (void*)ext2_unlink;
	fs->rename = (void*)ext2_rename;
	fs->create = (void*)ext2_create;
	fs->read = (void*)ext2_read;
	fs->write = (void*)ext2_write;
	fs->link = (void*)ext2_link;
	fs->rmdir = (void*)ext2_rmdir;
	// fs->symlink = (void*)ext2_symlink;
	fs->mkdir = (void*)ext2_mkdir;
	fs->truncate = (void*)ext2_truncate;
	fs->sync = (void*)ext2_sync;
	fs->fsync = (void*)ext2_fsync;

	return 0;
}

inode* ext2_open(const char* path, context* context)
{
	/* First lets try a query */
	inode query_node;
	strncpy(query_node.path, path, EXT2_MAX_PATH);
	query_node.inode_num = 0;
	query_node.inode_group = 0;
	inode* result = cache_query(&query_node, &context->inode_cache);
	if(result)
		return cache_reference(result->inode_num, 
				&context->inode_cache);

	/* We have never seen this node before. */
	int num = ext2_lookup(path, NULL, context);
	if(num < 0) return NULL;

	inode* ino = cache_reference(num, &context->inode_cache);
	if(!ino) return NULL;
	if(strlen(ino->path) == 0)
		strncpy(ino->path, path, EXT2_MAX_PATH);
	return ino;
}

int ext2_close(inode* ino, context* context)
{
	return cache_dereference(ino, &context->inode_cache);
}

int ext2_stat(inode* ino, struct stat* dst, context* context)
{
	uint_64 file_size = ino->ino->lower_size |
		((uint_64)ino->ino->upper_size << 32);

	memset(dst, 0, sizeof(struct stat));
	dst->st_dev = 0;
	dst->st_ino = ino->inode_num;
	dst->st_mode = ino->ino->mode;
	dst->st_nlink = ino->ino->hard_links;
	dst->st_uid = ino->ino->owner;
	dst->st_gid = ino->ino->group;
	dst->st_rdev = 0;
	dst->st_size = file_size;
	dst->st_blksize = context->blocksize;
	dst->st_blocks = ino->ino->sectors;
	dst->st_atime = ino->ino->last_access_time;
	dst->st_mtime = ino->ino->modification_time;
	dst->st_ctime = ino->ino->creation_time;

	return 0;
}

int ext2_create(const char* path, mode_t permissions, 
		uid_t uid, gid_t gid, context* context)
{
	/* Does this file already exist? */
	inode* file = ext2_open(path, context);
	if(file)
	{
		ext2_close(file, context);
		return -1;
	}

	/* Make sure this is just a normal file */
	permissions &= ~S_IFMT;
	permissions |= S_IFREG;
	/* Get the parent directory */
	char parent[EXT2_MAX_PATH];
	strncpy(parent, path, EXT2_MAX_PATH);
	if(file_path_parent(parent))
		return -1;
	if(file_path_file(parent))
		return -1;

	char name[EXT2_MAX_PATH];
	strncpy(name, path, EXT2_MAX_PATH);
	if(file_path_name(name)) return -1;

	inode* parent_inode = ext2_open(parent, context);

	int new_file = ext2_find_free_inode(
			parent_inode->inode_group, 0, context);
	if(new_file < 0) return -1;

	if(ext2_alloc_dirent(parent_inode->ino, new_file,
				parent_inode->inode_group, name, 
				EXT2_FILE_REG_FILE, context)) return -1;

	ext2_close(parent_inode, context); /* Clean up parent */

	inode* ino = ext2_open(path, context);
	if(!ino) return -1;
	/* Lets create a new inode */
	disk_inode* new_ino = ino->ino;
	memset(new_ino, 0, sizeof(disk_inode));

	/* update attributes */
	new_ino->mode = permissions;
	new_ino->owner = uid;
	new_ino->group = new_file >> context->inodegroupshift;
	new_ino->creation_time = 0;/* TODO: get kernel time */
	new_ino->modification_time = new_ino->creation_time;
	new_ino->hard_links = 1; /* Starts with one hard link */

	/* Close the child */
	ext2_close(ino, context);

	return 0;
}

int ext2_chown(inode* ino, uid_t uid, gid_t gid, context* context)
{
	ino->ino->owner = uid;
	ino->ino->group = gid;

	/* Results will get written to disk when the file is closed.*/
	return 0;
}

int ext2_chmod(inode* ino, mode_t mode, context* context)
{
	ino->ino->mode = mode;

	/* Results will get written to disk when the file is closed.*/
	return 0;
}

int ext2_truncate(inode* ino, int sz, context* context)
{
	return _ext2_truncate(ino->ino, sz, context);
}

int ext2_link(const char* file, const char* link, context* context)
{
	/* Get the parent directory */
	char parent[EXT2_MAX_PATH];
	strncpy(parent, link, EXT2_MAX_PATH);
	if(file_path_parent(parent))
		return -1;
	const char* name = link + (strlen(parent) + 1);
	if(!strcmp("/", parent)) name--; /* compensate for root */

	inode* parent_inode = ext2_open(parent, context);

	int new_file = ext2_lookup(file, NULL, context);
	if(new_file < 0) return -1;

	if(ext2_alloc_dirent(parent_inode->ino, new_file,
				parent_inode->inode_group, name, EXT2_FILE_REG_FILE,
				context)) return -1;

	ext2_close(parent_inode, context); /* Clean up parent */

	/* Lets update references */
	inode* file_ino = ext2_open(link, context);
	if(!file_ino)
	{
		cprintf("ext2: failed to create hard link.\n");
		cprintf("ext2: file: %s  link: %s.\n",
				file, link);
		return -1;
	}
	file_ino->ino->hard_links++;

	/* Flush new link to disk */
	ext2_close(file_ino, context);

	return 0;
}

int ext2_symlink(const char* file, const char* link, context* context)
{
	/* Get the parent directory */
	char parent[EXT2_MAX_PATH];
	strncpy(parent, file, EXT2_MAX_PATH);
	if(file_path_parent(parent))
		return -1;
	const char* name = link + (strlen(parent) + 1);

	inode* parent_inode = ext2_open(parent, context);

	int new_file = ext2_find_free_inode(
			parent_inode->inode_group, 0, context);
	if(new_file < 0) return -1;

	if(ext2_alloc_dirent(parent_inode->ino, new_file,
				parent_inode->inode_group, name, EXT2_FILE_REG_FILE,
				context)) return -1;

	ext2_close(parent_inode, context); /* Clean up parent */

	/* Lets write the path */
	inode* file_ino = ext2_open(link, context);
	if(!file_ino)
	{
		cprintf("ext2: failed to create soft link.\n");
		cprintf("ext2: file: %s  link: %s.\n",
				file, link);
		return -1;
	}
	if(ext2_write(file_ino, file, 0, strlen(file) + 1, context)
			!= strlen(file) + 1)
		return -1;

	/* Flush new link to disk */
	ext2_close(file_ino, context);

	return 0;
}

int ext2_mkdir(const char* path, mode_t permission, 
		uid_t uid, gid_t gid, context* context)
{
	/* Try to create the file */
	if(ext2_create(path, permission, uid, gid, context))
		return -1;

	/* Lets open the file now */
	inode* ino = ext2_open(path, context);
	if(!ino) 
	{
		/* Delete the file */
		ext2_unlink(path, context);
		return -1;
	}

	/* Change the permission of the file to be a directory */
	permission &= ~S_IFMT;
	permission |= S_IFDIR;
	ino->ino->mode = permission;

	/* Change ownership */
	ino->ino->owner = uid;
	ino->ino->group = gid;

	/* Add the basic entries */
	char parent[EXT2_MAX_PATH];
	char file_name[EXT2_MAX_PATH];
	strncpy(parent, path, EXT2_MAX_PATH);
	strncpy(file_name, path, EXT2_MAX_PATH);
	if(file_path_parent(parent) && file_path_file(parent))
	{
		/* Delete the file */
		ext2_unlink(path, context);
		return -1;
	}

	if(file_path_name(file_name) && file_path_file(file_name))
	{
		/* Delete the file */
		ext2_unlink(path, context);
		return -1;
	}

	inode* parent_inode = ext2_open(parent, context);

	/* Make sure the parent exists */
	if(!parent_inode < 0)
	{
		/* Delete the file */
		ext2_unlink(path, context);
		return -1;
	}

	/* Create 2 entries */
	if(ext2_alloc_dirent(ino->ino, ino->inode_num,
				ino->inode_num >> context->inodegroupshift,
				".", EXT2_FILE_DIR, context))
	{
		/* Delete the file */
		ext2_unlink(path, context);
		return -1;
	}

	if(ext2_alloc_dirent(ino->ino, parent_inode->inode_num,
				ino->inode_num >> context->inodegroupshift,
				"..", EXT2_FILE_DIR, context))
	{
		/* Delete the file */
		ext2_unlink(path, context);
		return -1;
	}


	if(ext2_modify_dirent_type(parent_inode->ino, file_name, 
				parent_inode->inode_group, EXT2_FILE_DIR, context))
	{
		/* Delete the file */
		ext2_unlink(path, context);
		return -1;
	}

	/* Close parent */
	ext2_close(parent_inode, context);

	/* Close new directory */
	ext2_close(ino, context);

	return 0;
}

int ext2_rmdir(const char* path, context* context)
{
	/* Same thing as unlink for ext2 */
	return ext2_unlink(path, context);
}

int ext2_read(inode* ino, void* dst, uint start, uint sz, 
		context* context)
{
	return _ext2_read(dst, start, sz, ino->ino, context);
}

int ext2_write(inode* ino, const void* src, uint start, uint sz,
		context* context)
{
	return _ext2_write(src, start, sz, 
			ino->inode_group, ino->ino, context);
}

int ext2_rename(const char* src, const char* dst, context* context)
{
	/**
	 * TODO: if the src and dst are in the same directory,
	 * there is a huge performance gain that could be made
	 * here.
	 */

	/* Just create a hard link */
	if(ext2_link(src, dst, context)) return -1;
	/* Unlink old reference */
	if(ext2_unlink(src, context)) return -1;
	return 0;
}

int ext2_unlink(const char* file, context* context)
{
	char parent_path[EXT2_MAX_NAME];
	strncpy(parent_path, file, EXT2_MAX_NAME);
	if(file_path_parent(parent_path))
		return -1;
	char file_name[EXT2_MAX_NAME];
	strncpy(file_name, file, EXT2_MAX_NAME);
	if(file_path_name(file_name))
		return -1;

	/* Get the inode for the file */
	inode* ino = ext2_open(file, context);
	inode* parent = ext2_open(parent_path, context);

	if(!ino || !parent)
	{
		if(ino) ext2_close(ino, context);
		if(parent) ext2_close(parent, context);
	}

	/* Remove the directory entry */
	int success = ext2_free_dirent(parent->ino, parent->inode_group,
			file_name, context);
	/* Decrement hard links */
	ino->ino->hard_links--;

	if(!ino->ino->hard_links)
	{
		/* We can completely free this inode */
		success |= ext2_free_inode(ino->inode_num, 
				S_ISDIR(ino->ino->mode), context);

		/**
		 * TODO: all blocks belonging to this inode are
		 * leaked here.
		 */
	}

	ext2_close(ino, context);
	ext2_close(parent, context);
	return success;
}

int ext2_readdir(inode* dir, int index, struct dirent* dst, 
		context* context)
{
	uint_64 file_size = dir->ino->lower_size |
		((uint_64)dir->ino->upper_size << 32);

	struct ext2_dirent diren;

	int x;
	int pos;
	for(x = 0, pos = 0;x < index + 1 && pos < file_size;x++)
	{
		if(_ext2_readdir(&diren, pos, dir->ino, context))
			return -1;

		if(x == index)
		{
			/* convert ext2 dirent to dirent */
			memset(dst, 0, sizeof(struct dirent));
			dst->d_ino = diren.inode;
			dst->d_off = pos + diren.size;
			dst->d_reclen = sizeof(struct dirent);
			strncpy(dst->d_name, diren.name, FILE_MAX_NAME);

			return 0;
		}

		pos += diren.size;
	}

	return -1;
}

int ext2_getdents(inode* dir, struct dirent* dst_arr, uint count, 
		uint pos, context* context)
{
	uint_64 file_size = dir->ino->lower_size |
		((uint_64)dir->ino->upper_size << 32);

	if(pos >= file_size) return 0; /* End of directory */

	struct ext2_dirent diren;

	int x;
	int bytes_read = 0;
	for(x = 0;x < count && pos + bytes_read < file_size;x++)
	{
		if(_ext2_readdir(&diren, pos + bytes_read, 
					dir->ino, context))
			return -1;

		/* convert ext2 dirent to dirent */
		memset(dst_arr + x, 0, sizeof(struct dirent));
		dst_arr[x].d_ino = diren.inode;
		dst_arr[x].d_off = pos + bytes_read + diren.size;
		dst_arr[x].d_reclen = sizeof(struct dirent);
		strncpy(dst_arr[x].d_name, diren.name, FILE_MAX_NAME);

		/**
		 * This file will no doubt be accessed soon, so we
		 * are going to inform the cache that it should
		 * keep the inode reference warm.
		 */
		inode* prepare = cache_reference(diren.inode, 
				&context->inode_cache);

		if(prepare)
		{
			if(!strlen(prepare->path))
			{
				strncpy(prepare->path, 
						dir->path, EXT2_MAX_PATH);
				if(!file_path_dir(prepare->path, 
							EXT2_MAX_PATH))
				{
					strncat(prepare->path, diren.name,
							EXT2_MAX_PATH);	
				} else memset(prepare->path, 
						0, EXT2_MAX_PATH);
			}
			cache_dereference(prepare, &context->inode_cache);
		}

		bytes_read += diren.size;
	}

	return bytes_read;
}

void ext2_sync(context* context)
{
	/* Sync the superblock */
	char* super_buffer = context->super_block + context->super_offset;

	memmove(&context->base_superblock, super_buffer, 
			sizeof(struct ext2_base_superblock));
	if(context->base_superblock.major_version >= 1)
	{
		memmove(&context->extended_superblock, super_buffer
				+ sizeof(struct ext2_base_superblock),
				sizeof(struct ext2_extended_base_superblock));
	}

	/* Sync the data blocks */
	cache_sync_all(&context->driver->cache);
}

int ext2_fsync(inode* ino, context* context)
{
	return cache_sync(ino, &context->inode_cache);
}

int ext2_fsstat(struct fs_stat* dst, context* context)
{
	return -1;
}
