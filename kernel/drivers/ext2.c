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

struct ext2_cache_inode
{

};

#define EXT2_DIRECT_COUNT 12

struct ext2_linux_specific
{
	uchar fragment_number;
	uchar fragment_size;
	uint_16 reserved1;
	uint_16 userid_high;
	uint_16 groupid_high;
	uint_32 reserved2;
};

struct ext2_hurd_specific
{
        uchar fragment_number;
        uchar fragment_size;
        uint_16 mode_high;
        uint_16 userid_high;
        uint_16 groupid_high;
        uint_32 author_id;
};

struct ext2_masix_specific
{
        uchar fragment_number;
        uchar fragment_size;
	uchar reserved[10];
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
	char fs_uuid[16];
	char name[16];
	char last_mount[64];
	uint_32 compression_alg;
	uchar file_preallocate;
	uchar dir_preallocate;
	uint_16 unused1;
	char journal_id[16];
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

struct ext2_context
{
	struct ext2_base_superblock base_superblock;
};

typedef struct ext2_disk_inode disk_inode;
typedef struct ext2_cache_inode inode;
typedef struct ext2_context context;


int ext2_init(uint superblock_address, uint block_size,
		uint cache_sz, struct FSHardwareDriver* driver,
		struct FSDriver* fs)
{
	fs->driver = driver;
	fs->valid = 1;
	fs->type = 0x0; /* TODO: adjust for real type here */
	if(sizeof(context) > FS_CONTEXT_SIZE)
	{
		cprintf("EXT2 Fatal error: not enough context space.\n");
		return -1;
	}
	memset(fs->context, FS_CONTEXT_SIZE, 0);
	memset(fs->cache, FS_CACHE_SIZE, 0);
	// context* c = (void*)driver->context;	

	/* Lets get some information on the system */
	cprintf("Size of base superblocks: %d\n", sizeof(struct ext2_base_superblock) + sizeof(struct ext2_extended_base_superblock));

	/* Implemented functions here */

	return 0;
}

inode* ext2_opened(const char* path, context* context)
{
	return NULL;
}


inode* ext2_open(const char* path, context* context)
{
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

