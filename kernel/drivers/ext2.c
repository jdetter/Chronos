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
	int bitmapshift; /* Shift for bit maps */
	int bitmapcount; /* How many bits are in a bitmap? */
	int grouptableaddr; /* in which block is the bgdt? */
	int grouptableshift;
	int blockspergroup; /* how many blocks are in a group? */
	int groupcount; /* How many block groups are there? */
	int inodeblocks; /* How many blocks are inode blocks? (grp)*/
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

static int ext2_read_bgdt(int group, 
		struct ext2_block_group_table* dst, context* context)
{
	char block[context->blocksize];
        int block_group_addr = context->grouptableaddr;
        int offset = group << context->grouptableshift;
        block_group_addr += (offset >> context->blockshift);
        offset &= context->blocksize - 1;

        /* Read the block group */
        if(context->driver->readblock(block, block_group_addr, context->driver) != context->blocksize)
                return -1;
        struct ext2_block_group_table* table = (void*)(block + offset);
	memmove(dst, table, 1 << context->grouptableshift);

	return 0;
}

static int ext2_get_bit(char* block, int bit, context* context)
{
	uchar byte = bit >> 3; /* 8 bits in a byte */
        uchar bit_offset = bit & 7; /* Get last 3 bits */
	if(byte >= context->blocksize)
		return -1; /* Bit doesn't exist! */
	return (block[byte] >> bit_offset) & 0x1;
}

static int ext2_set_bit(char* block, int bit, int val, context* context)
{
        uchar byte = bit >> 3; /* 8 bits in a byte */
        uchar bit_offset = bit & 7; /* Get last 3 bits */
        if(byte >= context->blocksize)
                return -1; /* Bit doesn't exist! */
	block[byte] |= (val & 0x01) << bit_offset;
        return 0;
}

static int ext2_alloc_block(int block_group, int blockid, context* context)
{
	char block[context->blocksize];
	/* Get the block group descriptor table */
	struct ext2_block_group_table table;
	if(ext2_read_bgdt(block_group, &table, context))
		return -1;
	/* Is the block free? */
	int block_offset = blockid >> context->bitmapshift;
	int offset = blockid & (context->bitmapcount - 1);
	if(context->driver->readblock(block, 
				block_offset + table.block_bitmap_address,
				context->driver) != context->blocksize)
		return -1;

	/* Is the block already allocated? */
	if(ext2_get_bit(block, offset, context))
		return -1; /* Already allocated!*/

	/* Set the bit */
	if(ext2_set_bit(block, offset, 1, context))
		return -1;

	/* Write the updated bitmap */
	if(context->driver->writeblock(block,
                                block_offset + table.block_bitmap_address,                    
                                context->driver) != context->blocksize)
                return -1;

	/* Read the block group table */
	/* TODO: update block group table metadata (free blocks - 1)*/

	return 0;
}

static int ext2_find_free_blocks(int group, int hint, 
		int contiguous, context* context)
{
	struct ext2_block_group_table table;
	char block[context->blocksize];

	/* Read the table descriptor */
	if(ext2_read_bgdt(group, &table, context))
		return -1;

	if(context->driver->readblock(block, table.block_bitmap_address, 
		context->driver) != context->blocksize)
		return -1;
	int sequence = 0; /* How many free in a row have we found? */
	int range_start = -1;
	int x; /* The bit in the group */
	for(x = 0;x < context->blockspergroup;x++)
	{
		/* Are there any free blocks in this byte? */
		if(!(x & 7) /* Is this a new byte? */
			&& block[x & ~7] == 0xFF)
		{
			/* This byte is completely allocated */
			x += 7;
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
	if(sequence != contiguous) return -1;

	/* Allocate the range */
	for(x = 0;x < sequence;x++)
	{
		if(ext2_alloc_block(group, range_start + x, context))
			return -1;
	}

	return range_start;
}

/**
 * Quickly find any free block
 */
static int ext2_find_free_block(int hint, context* context)
{
	int x = 0;
	int block = -1;
	if(hint > 0)x = hint;
	for(;x < context->groupcount && block <= 0;x++)
	{
		block = ext2_find_free_blocks(x, -1, 1, context);
		if(block > 0) break;
	}
	if(hint > 0 && block <= 0) /* that hint was bad */
		return(ext2_find_free_block(-1, context));
	struct ext2_block_group_table table;
	if(ext2_read_bgdt(x, &table, context))
		return -1;

	if(block > 0) 
	{
		ext2_alloc_block(x, block, context);
		/* Update the table */
		/* TODO: update the table */

		return block + table.inode_table + context->inodeblocks;
	}


	return -1;
}

static int ext2_read_inode(disk_inode* dst, int num, context* context)
{
	char block[context->blocksize];
	if(num <= 0) return -1;
	int block_group = (num - 1) >> context->inodegroupshift;
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
	context->driver->readblock(block, inode_address, context->driver);
	disk_inode* ino = (void*)(block + inode_block_offset);
	memmove(dst, ino, sizeof(disk_inode));

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

/**
 * Set the logical block address for a block in an inode.
 * TODO: there are a lot of performance improvements that can be made here.
 */
static int ext2_set_block_address(int index, int val, int block_hint,
		disk_inode* ino, context* context)
{
	char block[context->blocksize];
	int* i_block = (int*)block;
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
			indirect = ext2_find_free_block(block_hint, 
					context);
			if(indirect < 0)
				return -1;
			/* update inode */
			ino->indirect = indirect;
			/* zero the block*/
			memset(block, 0, context->blocksize);
		} else {
			if(context->driver->readblock(block,
						indirect, context->driver)
					!= context->blocksize)
				return -1;
		}

		i_block[index] = val;
		if(context->driver->writeblock(block, indirect, context->driver) 
				!= context->blocksize)
			return -1;
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
			dindirect = ext2_find_free_block(block_hint, 
					context);
			if(dindirect < 0)
				return -1;

			/* update inode pointer */
			ino->dindirect = dindirect;
			/* SECURITY OVER PERFORMANCE: zero the block*/
			memset(block, 0, context->blocksize);
		} else {
			if(context->driver->readblock(block,
						ino->dindirect,context->driver)
					!= context->blocksize)
				return -1;
		}

		int upper = (index >> context->indirectshift)
			& (context->addrs_per_block - 1);
		int lower = index & (context->addrs_per_block - 1);

		int indirect = i_block[upper];

		/* Do we have an indirect? */
		if(indirect == 0)
		{
			/* Find any free block */
			indirect = ext2_find_free_block(block_hint,
					context);
			if(indirect < 0)
				return -1;
			/* update the block*/
			i_block[upper] = indirect;
			context->driver->writeblock(block,
					dindirect, context->driver);

			/* SECURITY OVER PERFORMANCE: zero the block*/
			memset(block, 0, context->blocksize);
		} else {
			if(context->driver->readblock(block,
						indirect, context->driver)
					!= context->blocksize)
				return -1;
		}

		i_block[lower] = val;
		/* update indirect */
		if(context->driver->writeblock(block,
					indirect, context->driver)
				!= context->blocksize)
			return -1;
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
			tindirect = ext2_find_free_block(block_hint,
					context);
			if(tindirect < 0)
				return -1;

			/* update inode */
			ino->tindirect = tindirect;
			/* zero the block*/
			memset(block, 0, context->blocksize);
		} else {
			if(context->driver->readblock(block,
						tindirect, context->driver)
					!= context->blocksize)
				return -1;
		}

		int dindirect = i_block[upper];
		/* Do we have a dindirect? */
		if(!dindirect)
		{
			/* Find any free block */
			dindirect = ext2_find_free_block(block_hint,
					context);
			if(dindirect < 0)
				return -1;

			i_block[upper] = dindirect;
			if(context->driver->writeblock(block, 
						ino->tindirect,context->driver)
					!= context->blocksize)
				return -1;
			memset(block, 0, context->blocksize);
		} else {
			if(context->driver->readblock(block,
						tindirect, context->driver)
					!= context->blocksize)
				return -1;
		}

		int indirect = i_block[middle];

		/* Do we have an indirect? */
		if(!indirect)
		{
			/* Find any free block */
			indirect = ext2_find_free_block(block_hint,
					context);
			if(indirect < 0)
				return -1;

			/* SECURITY OVER PERFORMANCE: zero the block*/
			i_block[middle] = indirect;
			if(context->driver->writeblock(block, 
						dindirect, context->driver)
					!= context->blocksize)
				return -1;
			memset(block, 0, context->blocksize);
		} else {
			if(context->driver->readblock(block,
						indirect, context->driver)
					!= context->blocksize)
				return -1;
		}

		i_block[lower] = val;
		/* write back the block */
		if(context->driver->writeblock(block, indirect, 
					context->driver)
				!= context->blocksize)
			return -1;

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

static int _ext2_write(void* src, uint start, uint sz,
		disk_inode* ino, context* context)
{
	/* 
	 * If we're writing past the end of the file, we 
	 * need to add blocks where were writing
	 */
	uint_64 file_size = ino->lower_size |
		((uint_64)ino->upper_size << 32);
	if(start + sz > file_size)
	{
		/* We may need to allocate blocks */
		int required_blocks = (start + sz) >> context->blockshift;
		int blocks = file_size >> context->blockshift;
		int blocks_to_add = required_blocks - blocks;
	}

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

static int ext2_lookup_rec(const char* path, struct ext2_dirent* dst, 
		disk_inode* handle, context* context)
{
	uint_64 file_size = handle->lower_size | 
		((uint_64)handle->upper_size << 32);
	char parent[EXT2_MAX_PATH];
	struct ext2_dirent dir;

	/* First, kill prefix slashes */
	while(*path == '/')path++;
	if(file_path_root(path, parent, EXT2_MAX_PATH)) return -1;
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
				disk_inode new_handle;
				if(ext2_read_inode(&new_handle, 
							dir.inode, context))
					return -1;
				if(S_ISDIR(new_handle.mode))	
					return ext2_lookup_rec(path, dst, 
							&new_handle,context);
			}
		}

		pos += dir.size;
	}

	return -1; /* didn't find anything! */
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
	if(ext2_read_inode(dst, ino, context) < 0)
		return -1;
	return ino;

}

inode* ext2_opened(const char* path, context* context);
inode* ext2_open(const char* path, context* context);
void ext2_close(inode* ino, context* context);
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
int ext2_write(inode* ino, void* src, uint start, uint sz,
		context* context);
int ext2_rename(const char* src, const char* dst, context* context);
int ext2_unlink(const char* file, context* context);
int ext2_readdir(inode* dir, int index, struct dirent* dst,
		context* context);
int ext2_fsstat(struct fs_stat* dst, context* context);
int ext2_getdents(inode* dir, struct dirent* dst_arr, uint count,
		uint pos, context* context);

int ext2_test(context* context)
{
	disk_inode ino;
	int inonum = ext2_lookup_inode("/bin/bash", &ino, context);
	cprintf("Inode number for /bin/bash: %d\n", inonum);

	ext2_cache_free(context->root, context);
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
	context->cache_count = cache_sz / sizeof(inode);
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
	context->bitmapshift = context->blockshift << 2;
	context->bitmapcount = 1 << context->bitmapshift;
	context->blockspergroup = base->blocks_per_group;
	context->groupcount = base->block_count / 
		base->blocks_per_group;
	context->grouptableshift = 5; /* size = 32*/
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
		printf("Invalid revision type.\n");
		return -1;
	}

	context->inodesizeshift = log2(context->inodesize);
	if(context->inodesizeshift < 0) return -1;
	context->inodeblocks = (base->inodes_per_group << 
			context->inodesizeshift) >> 
		context->blockshift;

	/* Cache the root inode */
	inode* root = ext2_cache_alloc(context);
	strncpy(root->path, "/", 2);
	root->allocated = 1;
	root->references = 1;
	root->inode_num = 2;
	/* For whatever reason the root inode is always 2. */
	if(ext2_read_inode(&root->ino, 2, context))
		return -1;
	context->root = root;

	fs->open = (void*)ext2_open;
	fs->readdir = (void*)ext2_readdir;
	fs->close = (void*)ext2_close;
	fs->chmod = (void*)ext2_chmod;
	fs->chown = (void*)ext2_chown;
	fs->stat = (void*)ext2_stat;
	fs->read = (void*)ext2_read;
	fs->getdents = (void*)ext2_getdents;

	return 0;
}

inode* ext2_opened(const char* path, context* context)
{
	slock_acquire(&context->cache_lock);
	inode* ino = NULL;
	int x;
	for(x = 0;x < context->cache_count;x++)
	{
		if(!context->cache[x].allocated) continue;
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
	{
		slock_acquire(&context->cache_lock);
		/* Add a reference */
		if(ino->references != 0 && ino->allocated == 1)
			ino->references++;
		else {
			/* Someone deallocated this node! */
			ino->allocated = 0;
			slock_release(&context->cache_lock);
			return ext2_open(path, context);
		}
		slock_release(&context->cache_lock);
		return ino;
	}

	/* The file is not already opened. */
	ino = ext2_cache_alloc(context);
	strncpy(ino->path, path, FILE_MAX_PATH);
	ino->references++;

	/* Get the file metadata */
	int num = ext2_lookup_inode(path, &ino->ino, context);
	ino->inode_num = num; /* Save the inode number for later */

	return ino;
}

void ext2_close(inode* ino, context* context)
{
	if(!ino) return;
	ino->references--;
	/* Attempt to free the node if possible */
	ext2_cache_free(ino, context);
}

int ext2_stat(inode* ino, struct stat* dst, context* context)
{
	uint_64 file_size = ino->ino.lower_size |
		((uint_64)ino->ino.upper_size << 32);

	memset(dst, 0, sizeof(struct stat));
	dst->st_dev = 0;
	dst->st_ino = ino->inode_num;
	dst->st_mode = ino->ino.mode;
	dst->st_nlink = ino->ino.hard_links;
	dst->st_uid = ino->ino.owner;
	dst->st_gid = ino->ino.group;
	dst->st_rdev = 0;
	dst->st_size = file_size;
	dst->st_blksize = context->blocksize;
	dst->st_blocks = ino->ino.sectors;
	dst->st_atime = ino->ino.last_access_time;
	dst->st_mtime = ino->ino.modification_time;
	dst->st_ctime = ino->ino.creation_time;

	return 0;
}

int ext2_create(const char* path, mode_t permissions, 
		uid_t uid, gid_t gid, context* context)
{
	return -1;
}

int ext2_chown(inode* ino, uid_t uid, gid_t gid, context* context)
{
	ino->ino.owner = uid;
	ino->ino.group = gid;

	/* Results will get written to disk when the file is closed. */
	return 0;
}

int ext2_chmod(inode* ino, mode_t mode, context* context)
{
	ino->ino.mode = mode;

	/* Results will get written to disk when the file is closed. */
	return 0;
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
	return _ext2_read(dst, start, sz, &ino->ino, context);
}

int ext2_write(inode* ino, void* src, uint start, uint sz,
		context* context)
{
	return _ext2_write(src, start, sz, &ino->ino, context);
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
	uint_64 file_size = dir->ino.lower_size |
		((uint_64)dir->ino.upper_size << 32);

	struct ext2_dirent diren;

	int x;
	int pos;
	for(x = 0, pos = 0;x < index + 1 && pos < file_size;x++)
	{
		if(_ext2_readdir(&diren, pos, &dir->ino, context))
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
	uint_64 file_size = dir->ino.lower_size |
		((uint_64)dir->ino.upper_size << 32);

	if(pos >= file_size) return 0; /* End of directory */

	struct ext2_dirent diren;

	int x;
	int bytes_read = 0;
	for(x = 0;x < count && pos + bytes_read < file_size;x++)
	{
		if(_ext2_readdir(&diren, pos + bytes_read, 
					&dir->ino, context))
			return -1;

		/* convert ext2 dirent to dirent */
		memset(dst_arr + x, 0, sizeof(struct dirent));
		dst_arr[x].d_ino = diren.inode;
		dst_arr[x].d_off = pos + bytes_read + diren.size;
		dst_arr[x].d_reclen = sizeof(struct dirent);
		strncpy(dst_arr[x].d_name, diren.name, FILE_MAX_NAME);

		bytes_read += diren.size;
	}

	return bytes_read;
}

int ext2_fsstat(struct fs_stat* dst, context* context)
{
	return -1;
}
