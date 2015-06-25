#include "types.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "stdlock.h"
#include "vsfs.h"
#include "ata.h"
#include "panic.h"
#include "stdarg.h"
#include "stdlib.h"

/**
 * DEADLOCK NOTICE: in order to hold the itable lock, the fstable must be
 * acquired first. If the locks are not acquired in the correct order, it
 * could cause system deadlock. If you are holding the wrong lock, then you
 * must release that lock and then try to acquire both of them at the same
 * time.
 * 
 * Locking order:
 * 
 *   1) fstable_lock
 *   2) itable_lock
 *
 */

/* Global inode table */
tlock_t itable_lock;
struct inode_t itable[FS_INODE_MAX];

/* File system table */
tlock_t fstable_lock;
struct FSDriver fstable[FS_TABLE_MAX];

void fsman_init(void)
{
	int x;

	/* Initilize inode table */
	memset(&itable, 0, sizeof(struct inode_t) * FS_INODE_MAX);

	/* Bring up ata drivers */
	//ata_init(); this is now done in devman.

	/* Get a running driver */
	struct FSHardwareDriver* ata = NULL;
	for(x = 0;x < ATA_DRIVER_COUNT;x++)
		if(ata_drivers[x]->valid)
		{
			ata = ata_drivers[x];
			break;
		}
	if(ata == NULL) panic("No ata driver available.\n");

	/* Assign hardware driver */
	fstable[0].driver = ata;
	
	/* Set our vsfs as the root file system. */
	vsfs_driver_init(fstable);
	fstable[0].valid = 1;
	fstable[0].type = 0;
	fstable[0].driver = ata;
	strncpy(fstable[0].mount_point, "/", FILE_MAX_PATH);

	/* Initilize the file system */
	fstable[0].init(64, 0, 512, FS_CACHE_SIZE, 
		fstable[0].cache, fstable[0].context);
}

/**
 * Simplify the given path so that it doesn't contain any . or ..s
 * also check to make sure that the path is valid. Returns 1 if the
 * path has grammar errors. Returns 0 on success.
 */
static int fs_path_resolve(const char* path, char* dst, uint sz)
{
	int path_pos;
	int dst_pos;
	memset(dst, 0, sz);
	for(path_pos = 0, dst_pos = 0; 
			path_pos < strlen(path)
			&& dst_pos < sz;
			path_pos++, dst_pos++)
	{
		switch(path[path_pos])
		{
			/* Implement later */
			default:
				dst[dst_pos] = path[path_pos];
		}
	}
	return 0;
}

static struct FSDriver* fs_find_fs(const char* path)
{
	uint match_len = 0;
	struct FSDriver* fs = NULL;
	int x;
	for(x = 0;x < FS_TABLE_MAX;x++)
	{
		if(fstable[x].valid == 0) continue;
		uint mp_strlen = strlen(fstable[x].mount_point);
		if(!memcmp(fstable[x].mount_point, (char*)path, mp_strlen))
		{
			/* They are equal */
			if(match_len < mp_strlen)
			{
				match_len = mp_strlen;
				fs = fstable + x;
			}
		}
	}
	return fs;
}

/**
 * Find a free inode.
 */
static struct inode_t* fs_find_inode(void)
{
	/* Acquire itable lock */
	struct inode_t* result = NULL;
	int x;
	for(x = 0;x < FS_INODE_MAX;x++)
	{
		if(itable[x].valid == 0)
		{
			itable[x].valid = 1;
			result = itable + x;
			break;
		}
	}

	return result;
}

/**
 * Free an inode that is no longer in use. 
 */
static void fs_free_inode(inode i)
{
	memset(i, 0, sizeof(struct inode_t));
}

static int fs_get_name(const char* path, char* dst, uint sz)
{
	uint index = strlen(path);
	for(;index >= 0 && path[index] != '/';index--);
	index++;
	if(index < 0) return 1; /* invalid path */
	strncpy(dst, path + index, sz);
	return 0; 
}

static void fs_sync_inode(inode i)
{
	/* Get a stat structure */
	struct file_stat s;
	i->fs->stat(i->inode_ptr, &s, i->fs->context);
	i->file_sz = s.sz;
	i->inode_num = s.inode;
	i->file_type = s.type;
}

/**
 * BEGIN FSMAN FUNCTIONS
 *
 * These are the global functions that the operating system
 * will use to interact with the disks/file systems. These
 * functions are an abstraction that can be used with any
 * file system type. All constants for the flags and modes
 * defined here are in include/file.h.
 */

inode fs_open(const char* path, uint flags, uint permissions,
	uint uid, uint gid)
{
	/* TODO: Check if the file is already open by another process */	

	/* We need to use the driver function for this. */
	char dst_path[FILE_MAX_PATH];
	memset(dst_path, 0, FILE_MAX_PATH);
	if(fs_path_resolve(path, dst_path, FILE_MAX_PATH))
		return NULL; /* Invalid path */

	/* Find the file system for this path */
	struct FSDriver* fs = fs_find_fs(path);
	if(fs == NULL)
		return NULL; /* Invalid path */

	/* Try creating the file first */
	if(flags & O_CREATE)
		if(fs->create(path, permissions, uid, gid, fs->context))
			return NULL; /* Permission denied */

	struct inode_t* i = fs_find_inode();
	if(i == NULL)
		return NULL; /* No more inodes are available */
	i->fs = fs;

	i->inode_ptr = fs->open(path, fs->context);
	if(i->inode_ptr == NULL)
	{
		fs_free_inode(i);
		return NULL; /* Permission denied */
	}	

	/* Lets quickly get the stats on the file */
	struct file_stat st;
	fs->stat(i->inode_ptr, &st, fs->context);

	i->file_pos = 0;
	i->file_sz = st.sz;
	i->file_type = st.type;
	i->inode_num = st.inode;
	i->references = 1;
	fs_get_name(dst_path, i->name, FILE_MAX_NAME);

	return i;
}

int fs_close(inode i)
{
	/* Close the file system file */
	int result = i->fs->close(i->inode_ptr, i->fs->context);
	i->references--;	
	if(i->references == 0) 
		memset(i, 0, sizeof(struct inode_t));
	return result;
}

int fs_stat(inode i, struct file_stat* dst)
{
	/* This is completely file system specific. */
	int result = i->fs->stat(i->inode_ptr, dst, i->fs->context);
	dst->inode = i->inode_num;
	return result;
}

inode fs_create(const char* path, uint flags, 
		uint permissions, uint uid, uint gid)
{
	/* fix up the path*/
	char dst_path[FILE_MAX_PATH];
	memset(dst_path, 0, FILE_MAX_PATH);
	fs_path_resolve(path, dst_path, FILE_MAX_PATH);

	/* Find the file system this file belongs to */
	struct FSDriver* fs = fs_find_fs(path);
	if(fs == NULL) return NULL; /* invalid path */	

	/* Find a new inode*/
	struct inode_t* i = fs_find_inode();
	if(i == NULL) return NULL; /* There are no more free inodes */

	/* Create the file with the driver */
	if(fs->create(dst_path, 
                permissions, uid, gid, fs->context))
		return NULL;
	i->inode_ptr = fs->open(dst_path, fs->context);
	if(i->inode_ptr == NULL)
	{
		fs_free_inode(i);
		return NULL;
	}

	/* Parse the inode structure */
	struct file_stat st;
        fs->stat(i->inode_ptr, &st, fs->context);

        i->file_pos = 0;
        i->file_sz = st.sz;
        i->file_type = st.type;
        i->inode_num = st.inode;
        i->references = 1;
        fs_get_name(i->name, dst_path, FILE_MAX_NAME);

	return i;
}

int fs_chown(inode i, uint uid, uint gid)
{
	/* This is completely file system specific */
	return i->fs->chown(i->inode_ptr, uid, gid, i->fs->context);
}

int fs_chmod(inode i, ushort permission)
{
	/* This is file system specific */
	return i->fs->chmod(i->inode_ptr, permission, i->fs->context);
}

int fs_sync(inode i)
{
	/**
	 * We aren't doing any caching yet, but this is where the cache
	 * gets flushed to the disk if the user requests it.
	 */
	return 0;
}

int fs_truncate(inode i, int sz)
{
	int result = i->fs->truncate(i->inode_ptr, 
		sz, i->fs->context);
	if(result < 0) return -1;
	else i->file_sz = result;
	return result;
}

int fs_link(const char* file, const char* link)
{
	//int result = i->fs->link(i->inode_ptr,
	//	file, link, i->fs->context);
	return 0;
}

int fs_symlink(const char* file, const char* link)
{
        //int result = i->fs->symlink(i->inode_ptr,
        //        file, link, i->fs->context);
        return 0;
}

inode fs_mkdir(const char* path, uint flags, 
		uint permissions, uint uid, uint gid)
{
	/* First resolve the path */
	char dst_path[FILE_MAX_PATH];
	int result = fs_path_resolve(path, dst_path, FILE_MAX_PATH);
	if(result)
		return NULL; /* Bad path */
	
	/* Find the file system */
	struct FSDriver* fs = fs_find_fs(dst_path);
	
	/* Try to create the directory */
	if(fs->mkdir(path, permissions, uid, gid, fs->context))
		return NULL;
	void* dir = fs->open(path, fs->context);
	if(dir == NULL)
		return NULL; /* Bad path*/

	/* Create the inode */
	inode i = fs_find_inode();
	if(i == NULL) return NULL; /* Out of free inodes */
	i->inode_ptr = dir;

        /* Parse the inode structure */
        struct file_stat st;
        fs->stat(dir, &st, fs->context);

        i->file_pos = 0;
        i->file_sz = st.sz;
        i->file_type = st.type;
        i->inode_num = st.inode;
        i->references = 1;
        fs_get_name(i->name, dst_path, FILE_MAX_NAME);

	return i;
}

int fs_read(inode i, void* dst, uint sz, uint start)
{
	int bytes = i->fs->read(i->inode_ptr, dst, start, sz, i->fs->context);
	/* Check for read error */
	if(bytes < 0) return -1;

	/* Update our file position */
	i->file_pos += bytes;
	return bytes;
}

int fs_write(inode i, void* src, uint sz, uint start)
{
        int bytes = i->fs->write(i->inode_ptr,
                src, start, sz, i->fs->context);
        /* Check for read error */
        if(bytes < 0) return -1;

        /* Update our file position */
        i->file_pos += bytes;

	/* The file properties may have changed */
	fs_sync_inode(i);

	return bytes;
}

int fs_rename(const char* src, const char* dst)
{
	/**
	 * This is a simple operation that is usually done without even
	 * changing the inode information.
	 */

	/* Resolve both paths before sending them to the driver function. */
	char src_resolved[FILE_MAX_PATH];
	char dst_resolved[FILE_MAX_PATH];

	if(fs_path_resolve(src, src_resolved, FILE_MAX_PATH))
		return -1; /** Bad src path */
	if(fs_path_resolve(dst, dst_resolved, FILE_MAX_PATH))
		return -1; /* Bad dst path */
	
	struct FSDriver* src_fs = fs_find_fs(src_resolved);
	struct FSDriver* dst_fs = fs_find_fs(dst_resolved);

	/**
	 * If the two file systems are not equal, we can't just update
	 * the pointer. We need to create a new file and move the data.
	 */

	int result;

	if(src_fs != dst_fs)
	{
		result = -1;
	} else {
		result = src_fs->rename(src, dst, src_fs->context);
	}

	return result;
}

int fs_unlink(const char* file)
{
	/* Resolve the file path */
	char file_resolved[FILE_MAX_PATH];
	if(fs_path_resolve(file, file_resolved, FILE_MAX_PATH))
		return -1;

	struct FSDriver* fs = fs_find_fs(file_resolved);
	int result = fs->unlink(file_resolved, fs->context);

	return result;
}

int fs_readdir(inode i, int index, struct directent* dst)
{
	return i->fs->readdir(i->inode_ptr, index, dst, i->fs->context);
}

int fs_mount(const char* device, const char* point)
{
	return 0;	
}

