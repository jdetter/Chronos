#include "types.h"
#include "file.h"
#include "fsman.h"
#include "vsfs.h"
#include "ata.h"
#include "panic.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdlock.h"

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
struct filesystem_entry fstable[FS_TABLE_MAX];

/* Ata drivers */
static struct FSHardwareDriver* ata_drivers[];

/* Generic inode table */
void fsman_init(void)
{
	int x;

	/* Initilize locks */
	tlock_init(&itable_lock);
	tlock_init(&fstable_lock);

	/* Acquire locks */
	tlock_acquire(&itable_lock);
	tlock_acquire(&fstable_lock);

	/* Initilize inode table */
	memset(&itable, 0, sizeof(struct inode_t) * FS_INODE_MAX);

	/* Bring up ata drivers */
	ata_init();

	/* Get a running driver */
	struct FSHardwareDriver* ata = NULL;
	for(x = 0;x < ATA_DRIVER_COUNT;x++)
		if(ata_drivers[x]->valid)
			ata = ata_drivers[x];
	if(ata == NULL) panic("No ata driver available.\n");

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

static struct filesystem_entry* fs_find_fs(const char* path)
{
	uint match_len = 0;
	struct filesystem_entry* fs = NULL;
	int x;
	for(x = 0;x < FS_TABLE_MAX;x++)
	{
		uint mp_strlen = strlen(fstable[x].mount_point);
		if(memcmp(fstable[x].mount_point, (char*)path, mp_strlen))
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
	if(index < 0) return 1; /* invalid path */
	strncpy(dst, path + index, sz);
	return 0; 
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

uint fs_seek(inode i, int pos, uchar whence)
{
	switch(whence)
	{
		case SEEK_SET:
			i->file_pos = pos;
			break;
		case SEEK_END:
			i->file_pos = i->file_sz - pos;
			break;
		case SEEK_CUR:
			i->file_pos += pos;
			break;
	}

	return i->file_pos;
}

inode fs_open(const char* path, uint permissions, uint flags)
{
	/* We need to use the driver function for this. */
	char dst_path[FILE_MAX_PATH];
	memset(dst_path, 0, FILE_MAX_PATH);
	if(fs_path_resolve(path, dst_path, FILE_MAX_PATH))
		return NULL; /* Invalid path */

	/* Find the file system for this path */
	struct filesystem_entry* fs = fs_find_fs(path);
	if(fs == NULL)
		return NULL; /* Invalid path */

	struct inode_t* i = fs_find_inode();

	if(i == NULL)
		return NULL; /* No more inodes are available */

	i->inode_ptr = fs->driver->open(path, permissions, 
		flags, fs->context);
	if(i->inode_ptr == NULL)
	{
		fs_free_inode(i);
		return NULL; /* Permission denied */
	}	

	/* Lets quickly get the stats on the file */
	struct file_stat st;
	fs->driver->stat(i->inode_ptr, &st, fs->context);

	i->file_pos = 0;
	i->file_sz = st.sz;
	i->file_type = st.type;
	i->inode_num = st.inode;
	i->references = 1;
	fs_get_name(i->name, dst_path, FILE_MAX_NAME);

	return i;
}

int fs_close(inode i)
{
	/* Close the file system file */
	int result = i->fs->driver->close(i->inode_ptr, i->fs->context);
	i->references--;	
	if(i->references == 0) 
		memset(i, 0, sizeof(struct inode_t));
	return result;
}

int fs_stat(inode i, struct file_stat* dst)
{
	/* This is completely file system specific. */
	return i->fs->driver->stat(i->inode_ptr, dst, i->fs->context);
}

inode fs_create(const char* path, uint flags, uint permissions)
{
	/* fix up the path*/
	char dst_path[FILE_MAX_PATH];
	memset(dst_path, 0, FILE_MAX_PATH);
	fs_path_resolve(path, dst_path, FILE_MAX_PATH);

	/* Find the file system this file belongs to */
	struct filesystem_entry* fs = fs_find_fs(path);
	if(fs == NULL) return NULL; /* invalid path */	

	/* Find a new inode*/
	struct inode_t* i = fs_find_inode();
	if(i == NULL) return NULL; /* There are no more free inodes */

	/* Create the file with the driver */
	i->inode_ptr = fs->driver->create(dst_path, 
		flags, permissions, fs->context);
	if(i->inode_ptr == NULL)
	{
		fs_free_inode(i);
		return NULL;
	}

	/* Parse the inode structure */
	struct file_stat st;
        fs->driver->stat(i->inode_ptr, &st, fs->context);

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
	return i->fs->driver->chown(i->inode_ptr, uid, gid, i->fs->context);
}

int fs_chmod(inode i, ushort permission)
{
	/* This is file system specific */
	return i->fs->driver->chmod(i->inode_ptr, permission, i->fs->context);
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
	int result = i->fs->driver->truncate(i->inode_ptr, 
		sz, i->fs->context);
	if(result < 0) return -1;
	else i->file_sz = result;
	return result;
}

int fs_link(inode i, const char* file, const char* link)
{
	int result = i->fs->driver->link(i->inode_ptr,
		file, link, i->fs->context);
	return result;
}

int fs_symlink(inode i, const char* file, const char* link)
{
        int result = i->fs->driver->symlink(i->inode_ptr,
                file, link, i->fs->context);
        return result;
}

inode fs_mkdir(const char* path, uint flags, uint permissions)
{
	/* First resolve the path */
	char dst_path[FILE_MAX_PATH];
	int result = fs_path_resolve(path, dst_path, FILE_MAX_PATH);
	if(result)
		return NULL; /* Bad path */
	
	/* Find the file system */
	struct filesystem_entry* fs = fs_find_fs(dst_path);
	
	/* Try to create the directory */
	void* dir = fs->driver->mkdir(path, flags, permissions, fs->context);
	if(dir == NULL)
		return NULL; /* Bad path*/

	/* Create the inode */
	inode i = fs_find_inode();
	if(i == NULL) return NULL; /* Out of free inodes */
	i->inode_ptr = dir;

        /* Parse the inode structure */
        struct file_stat st;
        fs->driver->stat(dir, &st, fs->context);

        i->file_pos = 0;
        i->file_sz = st.sz;
        i->file_type = st.type;
        i->inode_num = st.inode;
        i->references = 1;
        fs_get_name(i->name, dst_path, FILE_MAX_NAME);

	return i;
}

/*
close: close an open file
create: create a file
chown: change ownership of file
chmod: change permissions of file
stat: get stat on file
sync: flush file to disk
truncate: truncate a file to a length
link: create a hard link
symlink: make a symbolic link
mkdir: create a directory
mknod: make a device node
read: read from a file
readdir: read a directory entry
rename: move a file
rmdir: remove a directory
unlink: delete a file reference
mount: create a new file system instance
unmount: close file system instance
*/

/* Begin vsfs manager functions */
