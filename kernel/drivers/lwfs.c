#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "kern/types.h"
#include "panic.h"
#include "cacheman.h"
#include "stdlock.h"
#include "file.h"
#include "vm.h"
#define NO_DEFINE_INODE
#include "fsman.h"
#undef NO_DEFINE_INODE

typedef uint32_t blkid_t;
typedef uint32_t sect_t;

#ifndef PGSIZE
#error "Compiling for arch with no PGSIZE?"
#endif

#define BSIZE PGSIZE
#define LWFS_DIRECT 8
#define LWFS_DIRNAMESZ 208

#define KTIME_PROVIDED
#define DEBUG

#ifdef BOOT_STRAP
#undef DEBUG
#endif

#ifdef KTIME_PROVIDED
#include "rtc.h"
#include "ktime.h"
#endif

struct lwfs_inode
{
	uint16_t mode;
	uint16_t owner_uid;
	uint16_t group_gid;
	uint16_t hard_links;
	uint32_t status_changed_time;
	uint32_t modification_time;
	uint32_t creation_time;
	uint32_t last_access_time;
	uint32_t size;
	void*    direct[LWFS_DIRECT];
	uint32_t next_free; /* The next free inode */
};

/* Copy of EXT2 file types */
#define LWFS_FILE_UNKNOWN       0x0
#define LWFS_FILE_REG_FILE      0x1
#define LWFS_FILE_DIR           0x2
#define LWFS_FILE_CHRDEV        0x3
#define LWFS_FILE_BLKDEV        0x4
#define LWFS_FILE_FIFO          0x5
#define LWFS_FILE_SOCK          0x6
#define LWFS_FILE_SYMLINK       0x7

struct lwfs_dirent
{
	uint32_t inode;
	uint8_t type;
	uint8_t name_sz;
	char name[LWFS_DIRNAMESZ];
};

typedef struct lwfs_context 	context;
typedef struct lwfs_inode 	inode;
typedef struct lwfs_dirent	dirent;

struct lwfs_context
{
        uintptr_t blocks_start; /* Where the data blocks start in memory */
        int block_count; /* The total amount of blocks available */
        uintptr_t inodes_start; /* Where the inode table starts in memory */
        int inode_count; /* The total amount of inodes available */

        /* Quick shifters */
        int block_shifter;
        int inode_shifter;
        int dirent_shifter;

	slock_t inode_list_lock;
	inode* inode_head; /* Next free inode */
	int free_inodes;
	slock_t block_list_lock;
	void* block_head; /* Next free block */
	int free_blocks;
};

int inode_to_num(inode* ino, context* c)
{
	inode* base = (inode*)c->inodes_start;
	return ino - base;
}

/* Allocate an inode */
inode* allocate_inode(context* c)
{
	slock_acquire(&c->inode_list_lock);

	inode* result = NULL;
	if(c->inode_head)
	{
		result = c->inode_head;
		c->inode_head = (inode*)result->next_free;
		memset(result, 0, sizeof(inode));
		c->free_inodes--;
	}

	slock_release(&c->inode_list_lock);

	return result;
}

/* Free an inode */
int free_inode(inode* ino, context* c)
{
	if((uintptr_t)ino >= c->inodes_start
		&& (uintptr_t)ino < c->inodes_start 
			+ (c->inode_count << c->inode_shifter))
	{
		slock_acquire(&c->inode_list_lock);
		ino->next_free = (uint32_t)c->inode_head;
		c->inode_head = ino;
		c->free_inodes++;
		slock_release(&c->inode_list_lock);

		return 0;
	}

	return -1;
}

/* Allocate a block */
void* allocate_block(context* c)
{
	slock_acquire(&c->block_list_lock);

        void* result = NULL;
        if(c->block_head)
        {
                result = c->block_head;
                c->block_head = *((void**) result);
		c->free_blocks--;
		memset(result, 0, PGSIZE);
        }

        slock_release(&c->block_list_lock);

        return result;
}

/* Free a block */
int free_block(void* block, context* c)
{
        if((uintptr_t)block >= c->blocks_start
                && (uintptr_t)block < c->blocks_start
                        + (c->block_count << c->block_shifter))
        {
                slock_acquire(&c->block_list_lock);
                *((void**)block) = c->block_head;
                c->block_head = block;
		c->free_blocks++;
                slock_release(&c->block_list_lock);

                return 0;
        }

        return -1;
}

int _read(inode* inode, void* dst, off_t start, size_t sz,
		context* context)
{
	/* Is the file large enough? */
	if(start > inode->size) return 0;
	if(start + sz > inode->size)
		sz = inode->size - start;

	int start_block = start >> context->block_shifter;
        int end_block = (start + sz - 1) >> context->block_shifter;
        int curr_block;

	/* First block */
        size_t copied = PGSIZE - (start & (PGSIZE - 1));
        if(copied > sz) copied = sz;

        char* src_c = (char*)inode->direct[start_block] +
                (start & (PGSIZE - 1));
        char* dst_c = dst;

        memmove(dst_c, src_c, copied);
        dst_c += copied;

        /* Are we done? */
        if(copied == sz) return sz;

        /* Copy middle blocks */
        for(curr_block = start_block + 1;curr_block < end_block;curr_block++)
        {
                memmove(dst_c, inode->direct[curr_block], PGSIZE);
                dst_c += PGSIZE;
                copied += PGSIZE;
        }

        if(copied == sz) return sz;

        /* copy last block */
        memmove(dst_c, inode->direct[end_block], sz - copied);

	return sz;
}

int _write(inode* inode, const void* src, off_t start, size_t sz, 
		context* context)
{
	/* Is the start within the max file size? */
	if(start > LWFS_DIRECT << context->block_shifter)
		return 0;
	/* Is the sz within the max file size? */
	if(start + sz > LWFS_DIRECT << context->block_shifter)
		sz = (LWFS_DIRECT << context->block_shifter) - start;
		
	/* Allocate blocks if needed */
	int start_block = start >> context->block_shifter;
	int end_block = (start + sz - 1) >> context->block_shifter;
	int curr_block;
	for(curr_block = 0;curr_block <= end_block;curr_block++)
	{
		if(!inode->direct[curr_block])
		{
			void* new_block = allocate_block(context);
			if(!new_block) return -1;
			inode->direct[curr_block] = new_block;
		}
	}

	/* Write to the blocks */

	/* First block */
	size_t copied = PGSIZE - (start & (PGSIZE - 1));
	if(copied > sz) copied = sz;

	const char* src_c = src;
	char* dst_c = (char*)inode->direct[start_block] + 
		(start & (PGSIZE - 1));

	memmove(dst_c, src, copied);
	src_c += copied;

	/* Are we done? */
	if(copied == sz) goto done;

	/* Copy middle blocks */
	for(curr_block = start_block + 1;curr_block < end_block;curr_block++)
	{
		memmove(inode->direct[curr_block], src_c, PGSIZE);
		src_c += PGSIZE;
		copied += PGSIZE;
	}

	if(copied == sz) goto done;

	/* copy last block */
	memmove(inode->direct[end_block], src_c, sz - copied);

done:
	/* Do we need to update the file size? */
	if(start + sz > inode->size) inode->size = start + sz;

	return sz;
}

int _truncate(inode* ino, size_t new_sz, context* context)
{
	if(new_sz >= ino->size) return new_sz;

	int new_start_block = (new_sz - 1) >> context->block_shifter;
	int old_start_block = (ino->size - 1) >> context->block_shifter;

	while(old_start_block > new_start_block)
	{
		void* block = ino->direct[old_start_block];
		if(free_block(block, context)) return -1;
		ino->direct[old_start_block] = 0;
		old_start_block--;
	}

	return new_sz;
}

int alloc_dirent(inode* dir, const char* name, inode* file, context* context)
{
	int ino_num = inode_to_num(file, context);
	/* Structure the new directory entry */
	dirent new_dirent;
	memset(&new_dirent, 0, sizeof(dirent));
	new_dirent.inode = ino_num;
	
	/* Determine the type */
	uint8_t type = LWFS_FILE_UNKNOWN;
	if(S_ISREG(file->mode))
		type = LWFS_FILE_REG_FILE;
	else if(S_ISDIR(file->mode))
		type = LWFS_FILE_DIR;
	else if(S_ISCHR(file->mode))
		type = LWFS_FILE_CHRDEV;
	else if(S_ISBLK(file->mode))
		type = LWFS_FILE_BLKDEV;
	else if(S_ISFIFO(file->mode))
		type = LWFS_FILE_FIFO;
	else if(S_ISLNK(file->mode))
		type = LWFS_FILE_SYMLINK;
	else if(S_ISSOCK(file->mode))
		type = LWFS_FILE_SOCK;
	new_dirent.type = type;
	int name_sz = strlen(name);
	if(name_sz > LWFS_DIRNAMESZ - 1)
		return -1; /* Name too long! */
	strncpy(new_dirent.name, name, name_sz);
	new_dirent.name_sz = name_sz;

	if(_write(dir, &new_dirent, file->size, sizeof(dirent), context)
				!= sizeof(dirent))
		return -1;
	return 0;
}

int free_dirent(inode* dir, const char* name, context* context)
{
	char found = 0;
	off_t offset;
	/* Find the directory entry we are replacing */
	for(offset = 0;offset < dir->size;offset += sizeof(dirent))
	{
		dirent curr_dirent;
		if(_read(dir, &curr_dirent, offset, sizeof(dirent), context)
					!= sizeof(dirent))
			return -1;

		if(!strcmp(curr_dirent.name, name))
		{
			/* Found it! */
			found = 1;
			break;
		}
	}

	if(!found) return -1;

	/* Is this the final directory entry? */
	if(offset + sizeof(dirent) == dir->size)
	{
		/* Just truncate the file */
		return _truncate(dir, offset, context) == offset;
	}

	/* Lets swap it with the last entry */
	dirent last_dirent;
	/* Grab the final directory entry */
	if(_read(dir, &last_dirent, dir->size - sizeof(dirent), 
				sizeof(dirent), context) != sizeof(dirent))
		return -1;
	/* Write it to the old directory entry */
	if(_write(dir, &last_dirent, offset, sizeof(dirent), context) 
			!= sizeof(dirent))
		return -1;

	/* Now truncate to the new size */
	return _truncate(dir, dir->size - sizeof(dirent), context)
			==  dir->size - sizeof(dirent);
}

void* lwfs_open(const char* path, context* context);
int lwfs_close(inode* i, context* context);
int lwfs_stat(inode* i, struct stat* dst, context* context);
int lwfs_create(const char* path, mode_t permission, uid_t uid, gid_t gid,
                context* context);
int lwfs_chown(inode* i, uid_t uid, gid_t gid, context* context);
int lwfs_chmod(inode* i, mode_t permission, context* context);
int lwfs_truncate(inode* i, size_t sz, context* context);
int lwfs_link(const char* file, const char* link, context* context);
int lwfs_symlink(const char* file, const char* link, context* context);
int lwfs_mkdir(const char* path, mode_t permission, uid_t uid, gid_t gid,
                context* context);
int lwfs_read(inode* inode, void* dst, uintptr_t start, size_t sz,
                context* context);
int lwfs_write(inode* ino, const void* src, intptr_t start, size_t sz,
                context* context);
int lwfs_rename(const char* src, const char* dst, context* context);
int lwfs_unlink(const char* file, context* context);
int lwfs_readdir(inode* dir, int index, struct dirent* dst, context* context);
int lwfs_getdents(inode* dir, struct dirent* dst_arr, size_t count,
                off_t posistion, context* context);
int lwfs_fsstat(struct fs_stat* dst, context* context);
inode* lwfs_opened(const char* path, context* context);
int lwfs_rmdir(const char* path, context* context);
int lwfs_mknod(const char* path, dev_t dev_major, dev_t dev_minor,
                mode_t permission, context* context);
void lwfs_sync(void* context);
int lwfs_fsync(void* i, void* context);
int fsck(context* context);


int lwfs_init(sect_t start_sector, sect_t end_sector, int sectsize,
		int cache_sz, char* cache, struct FSDriver* driver,
		void* c)
{
	context* context = c;
	memset(context, 0, sizeof(context));
	slock_init(&context->inode_list_lock);
	slock_init(&context->block_list_lock);

	/* Lets do some sanity checking: */
	size_t inode_sz = sizeof(inode);
	size_t dirent_sz = sizeof(dirent);
	size_t pg_sz = PGSIZE;

	int inode_shift = 0;
	int dirent_shift = 0;
	int block_shift = 0;

	/* Batch check all 3 of these to make sure they are powers of 2*/
	/* Also calculate shifts */
	int shift = 0;
	while(inode_sz | dirent_sz | pg_sz)
	{
		if(inode_sz == 1)
		{
			inode_shift = shift;
			inode_sz = 0;
		}

		if(dirent_sz == 1)
		{
			dirent_shift = shift;
			dirent_sz = 0;
		}

		if(pg_sz == 1)
		{
			block_shift = shift;
			pg_sz = 0;
		}

		if(inode_sz & 0x01) return -1;
		if(dirent_sz & 0x01) return -1;
		if(pg_sz & 0x01) return -1;

		inode_sz >>= 1;
		dirent_sz >>= 1;
		pg_sz >>= 1;
		shift++;
	}

#ifdef DEBUG
	cprintf("lwfs: inode size:   %d\n", (int)sizeof(inode));
	cprintf("lwfs: dirent size:  %d\n", (int)sizeof(dirent));
	cprintf("lwfs: context size: %d\n", (int)sizeof(context));
	cprintf("lwfs: page size:    %d\n", (int)PGSIZE);

	if(1 << block_shift != PGSIZE)
	{
		cprintf("lwfs: log2 algorithm is currupt!\n");
		return -1;
	}
#endif

	/* We shouldn't have a cache, we never use it. */
	if(cache_sz > 0)
	{
#ifdef DEBUG
		cprintf("lwfs: I don't need a cache!\n");
#endif
		cman_free(cache, cache_sz);
	}

	if(sectsize != PGSIZE)
	{
#ifdef DEBUG
		cprintf("lwfs: Sect size is imcompatable! have: %d need: %d\n",
				sectsize, PGSIZE);
#endif
		return -1;
	}

	/* How many inodes should we make? */
	int inode_blocks = 8;
	/* If we have more than a megabyte, allow more */
	if(end_sector > 0x1000000)
		inode_blocks = 16;
	/* If we have more than 256MB then allow a ton */
	if(end_sector > 0x100000000)
		inode_blocks = 32;

	size_t block_size = 1 << block_shift;
	context->inodes_start = start_sector << block_shift;
	context->inode_count = (block_size >> inode_shift) * inode_blocks;
	context->blocks_start = inode_blocks + context->inodes_start;
	context->block_count = end_sector - context->blocks_start;

	/* set quick shifters */
	context->block_shifter = block_shift;
	context->inode_shifter = inode_shift;
	context->dirent_shifter = dirent_shift;

	/* Add all free blocks */
	char* block_ptr_base = (void*)context->blocks_start;
	int block;
	for(block = 0;block < context->block_count;block++)
	{	
		if(free_block(block_ptr_base 
					+ (block << context->block_shifter), context))
			cprintf("Failed to add block: 0x%x\n", 
					block << context->block_shifter);
	}

	/* Add all free inodes */
	inode* inode_ptr_base = (inode*)context->inodes_start;
	int inum;
	for(inum = 0;inum < context->inode_count;inum++)
	{
		if(free_inode(inode_ptr_base + inum, context))
			cprintf("Failed to add inode: %d\n", inum);
	}

#ifdef DEBUG
	cprintf("lwfs: inodes: %d\n", context->inode_count);
	cprintf("lwfs: blocks: %d\n", context->block_count);

	cprintf("lwfs: Setup complete.\n");
#endif

	return 0;
}

void* lwfs_open(const char* path, context* context)
{
	return NULL;
}

int lwfs_close(inode* i, context* context)
{
	return -1;
}

int lwfs_stat(inode* i, struct stat* dst, context* context)
{
	dst->st_dev = 0;
	dst->st_ino = inode_to_num(i, context);
	dst->st_mode = i->mode;
	dst->st_nlink = i->hard_links;
	dst->st_uid = i->owner_uid;
	dst->st_gid = i->group_gid;
	dst->st_rdev = 0;
	dst->st_size = i->size;
	dst->st_blksize = PGSIZE;
	dst->st_blocks = (i->size + 511) >> 9;
	dst->st_atime = i->last_access_time;
	dst->st_mtime = i->modification_time;
	dst->st_ctime = i->status_changed_time;
	return -1;
}

int lwfs_create(const char* path, mode_t permission, uid_t uid, gid_t gid,
		context* context)
{
	char parent_path[FILE_MAX_PATH];
	char file_name[FILE_MAX_PATH];
	strncpy(parent_path, path, FILE_MAX_PATH);
	strncpy(file_name, path, FILE_MAX_PATH);

	if(file_path_parent(parent_path))
		return -1;
	if(file_path_file(parent_path))
		return -1;
	if(file_path_name(file_name))
		return -1;

	inode* parent_dir = lwfs_open(parent_path, context);
	inode* new_ino = allocate_inode(context);

	if(!parent_dir || !new_ino) 
	{
		if(parent_dir) lwfs_close(parent_dir, context);
		if(new_ino) free_inode(new_ino, context);
		return -1;
	}

	lwfs_chown(new_ino, uid, gid, context);
	lwfs_chmod(new_ino, permission, context);

	if(alloc_dirent(parent_dir, file_name, new_ino, context))
	{
		lwfs_close(parent_dir, context);
		free_inode(new_ino, context);
		return -1;
	}

	/* The inode is now a real file */
	new_ino->hard_links = 1;
	new_ino->modification_time = 0;
	new_ino->creation_time = 0;
	new_ino->status_changed_time = 0;
	new_ino->last_access_time = 0;
	new_ino->size = 0;
	memset(new_ino->direct, 0, sizeof(uint32_t) * LWFS_DIRECT);
	new_ino->next_free = 0;

#ifdef KTIME_PROVIDED
	new_ino->modification_time = ktime_seconds();
	new_ino->creation_time = new_ino->modification_time;
	new_ino->last_access_time = new_ino->modification_time;
	new_ino->status_changed_time = new_ino->modification_time;
#endif

	new_ino = NULL;
	lwfs_close(parent_dir, context);

	return 0;
}

int lwfs_chown(inode* i, uid_t uid, gid_t gid, context* context)
{
	i->owner_uid = uid;
	i->group_gid = gid;

#ifdef KTIME_PROVIDED
	i->status_changed_time = ktime_seconds();
#endif

	return 0;
}

int lwfs_chmod(inode* i, mode_t permission, context* context)
{
	/* dont allow the file to change type */
	permission &= 07777;

	/* Keep file type, but not sticky bit */
	i->mode &= ~07777;

	/* Assign new permission */
	i->mode |= permission;

#ifdef KTIME_PROVIDED
        i->status_changed_time = ktime_seconds();
#endif

	return 0;
}

int lwfs_truncate(inode* i, size_t sz, context* context)
{
	return _truncate(i, sz, context);
}

int lwfs_link(const char* file, const char* link, context* context)
{

	char link_parent[FILE_MAX_PATH];
	char link_name[FILE_MAX_PATH];

	strncpy(link_parent, link, FILE_MAX_PATH);
	strncpy(link_name, link, FILE_MAX_PATH);

	if(file_path_parent(link_parent))
		return -1;
	if(file_path_file(link_parent))
		return -1;
	if(file_path_name(link_name))
		return -1;

	inode* f = lwfs_open(file, context);
	inode* link_dir = lwfs_open(link_parent, context);

	if(!f || !link_dir)
	{
		if(f) lwfs_close(f, context);
		if(link_dir) lwfs_close(link_dir, context);
		return -1;
	}

	if(alloc_dirent(link_dir, link_name, f, context))
	{
		lwfs_close(f, context);
		lwfs_close(link_dir, context);
		return -1;
	}

#ifdef KTIME_PROVIDED	
	f->last_access_time = ktime_seconds();
#endif

	lwfs_close(f, context);
	lwfs_close(link_dir, context);

	return 0;
}

int lwfs_symlink(const char* file, const char* link, context* context)
{
	return -1;
}

int lwfs_mkdir(const char* path, mode_t permission, uid_t uid, gid_t gid,
		context* context)
{
	char parent_path[FILE_MAX_PATH];
	char dir_name[FILE_MAX_PATH];
	
	strncpy(parent_path, path, FILE_MAX_PATH);
	strncpy(dir_name, path, FILE_MAX_PATH);

	if(file_path_parent(parent_path))
		return -1;
	if(file_path_file(parent_path))
		return -1;
	if(file_path_name(dir_name))
		return -1;

	inode* parent = lwfs_open(parent_path, context);
        if(!parent) return -1;

	inode* new_dir = allocate_inode(context);
	if(!new_dir) 
	{
		lwfs_close(parent, context);
		return -1;
	}
	new_dir->mode = S_IFDIR;
	lwfs_chmod(new_dir, permission, context);
	lwfs_chown(new_dir, uid, gid, context);
	

	return -1;
}

int lwfs_read(inode* inode, void* dst, uintptr_t start, size_t sz, 
		context* context)
{
#ifdef KTIME_PROVIDED   
        inode->last_access_time = ktime_seconds();
#endif
	return _read(inode, dst, start, sz, context);
}

int lwfs_write(inode* ino, const void* src, intptr_t start, size_t sz,
		context* context)
{
#ifdef KTIME_PROVIDED   
        ino->last_access_time = ktime_seconds();
        ino->modification_time = ino->last_access_time;
#endif

	return _write(ino, src, start, sz, context);
}

int lwfs_rename(const char* src, const char* dst, context* context)
{
	if(lwfs_link(src, dst, context))
		return -1;
	lwfs_unlink(src, context);
	return 0;
}

int lwfs_unlink(const char* file, context* context)
{
	char parent_dir[FILE_MAX_PATH];
	char file_name[FILE_MAX_PATH];

	/* Calculate parent directory */
	strncpy(parent_dir, file, FILE_MAX_PATH);
	if(file_path_parent(parent_dir))
		return -1;
	if(file_path_file(parent_dir))
		return -1;

	/* Calculate file name */
	strncpy(file_name, file, FILE_MAX_PATH);
	if(file_path_name(file_name))
		return -1;

	inode* dir = lwfs_open(parent_dir, context);
	if(!dir) return -1;

	if(free_dirent(dir, file_name, context))
	{
		lwfs_close(dir, context);
		return -1;
	}

	lwfs_close(dir, context);

	/* Decrement references */
	inode* f = lwfs_open(file, context);
	if(!f) return -1;
	f->hard_links--;
	lwfs_close(f, context);
	
	return 0;
}

int lwfs_readdir(inode* dir, int index, struct dirent* dst, context* context)
{
	return -1;
}

int lwfs_getdents(inode* dir, struct dirent* dst_arr, size_t count,
		off_t posistion, context* context)
{
	return -1;
}

int lwfs_fsstat(struct fs_stat* dst, context* context)
{
	return -1;
}

inode* lwfs_opened(const char* path, context* context)
{
	return NULL;
}

int lwfs_rmdir(const char* path, context* context)
{
	return -1;
}

int lwfs_mknod(const char* path, dev_t dev_major, dev_t dev_minor,
		mode_t permission, context* context)
{
	return -1;
}

void lwfs_sync(void* context){}
int lwfs_fsync(void* i, void* context){return 0;}

int fsck(context* context)
{
	return -1;
}
