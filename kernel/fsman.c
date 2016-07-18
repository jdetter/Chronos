/**
 * Author: John Detter <john@detter.com>
 *
 * Chronos's file system manager. This handles all of the file system
 * operations.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>

#include "kstdlib.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "panic.h"
#include "stdarg.h"
#include "pipe.h"
#include "tty.h"
#include "chronos.h"
#include "proc.h"
#include "panic.h"
#include "storagecache.h"
#include "drivers/ext2.h"
#include "drivers/lwfs.h"

// #define DEBUG

#ifdef RELEASE
# undef DEBUG
#endif

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
slock_t itable_lock;
struct inode_t itable[FS_INODE_MAX];

/* File system table */
slock_t fstable_lock;
struct FSDriver fstable[FS_TABLE_MAX];

/* Global inode table */
extern slock_t itable_lock;
extern struct inode_t itable[];

/* File system table */
extern slock_t fstable_lock;
extern struct FSDriver fstable[];
char root_partition[FILE_MAX_PATH];

struct FSDriver* fs_alloc(void)
{
        slock_acquire(&fstable_lock);
        struct FSDriver* driver = NULL;
        int x;
        for(x = 0;x < FS_TABLE_MAX;x++)
        {
                if(!fstable[x].valid)
                {
#ifdef CACHE_WATCH
                        cprintf("FSDriver allocated: %d\n", x);
#endif
                        driver = fstable + x;
                        driver->valid = 1;
                        break;
                }
        }

#ifdef CACHE_PANIC
        if(!driver) panic("Too many file systems mounted!\n");
#endif

        slock_release(&fstable_lock);
        return driver;
}

void fs_free(struct FSDriver* driver)
{
        slock_acquire(&fstable_lock);
        driver->valid = 0;
        slock_release(&fstable_lock);
}

int fs_add_inode_reference(struct inode_t* i)
{
#ifdef CACHE_WATCH
	if(rproc) cprintf("INODE REFERENCE ADDED: %s process: %s pid: %d\n",
				i->name, rproc->name, rproc->pid);
	else cprintf("INODE REFERENCE ADDED: %s process: KERNEL\n",
				i->name);
#endif

	i->references++;
	return 0;
}



/**
 * Simplify the given path so that it doesn't contain any . or ..s
 * also check to make sure that the path is valid. Returns 1 if the
 * path has grammar errors. Returns 0 on success. The resulting
 * path will not end with a slash unless it is the root '/'
 */
int fs_path_resolve(const char* path, char* dst, size_t sz)
{
	if(strlen(path) == 0) return -1;
	int path_pos;
	int dst_pos = 0;
	memset(dst, 0, sz);
	if(path[0] != '/')
		dst_pos = strlen(strncpy(dst, rproc->cwd, sz));
	int dots = 0;
	int slashes = 0;
	for(path_pos = 0; 
			path_pos < strlen(path)
			&& dst_pos < sz;
			path_pos++, dst_pos++)
	{
		switch(path[path_pos])
		{
			case '.':
				dst[dst_pos] = '.';
				dots++;
				slashes = 0;
				break;
			case '/':
				if(slashes >= 1)
				{
					dst_pos--;
				} else if(dots == 0 && slashes == 0)
				{
					dst[dst_pos] = '/';
					slashes++;
				} else if(dots == 1)
				{
					/* delete previous dot */
					dst[dst_pos - 1] = 0;
					dst_pos -= 2;
					if(dst[dst_pos] != '/')
						return -1; /* bad syntax*/
				} else if(dots == 2)
				{
					/* delete both dots */
					dst[dst_pos - 1] = 0;
					dst[dst_pos - 2] = 0;
					dst_pos -= 3;
					if(dst[dst_pos] != '/')
						return -1; /* bad syntax*/
					if(dst_pos > 0)
					{
						dst[dst_pos] = 0;
						dst_pos --;
					}

					/* 
					 * Delete until you hit a slash or
					 * dst_pos == 1
					 */
					for(;dst_pos > 0 && 
						dst[dst_pos] != '/';
						dst_pos--)
						dst[dst_pos] = 0;
				} else {
					/* syntax error! */
					return -1;
				}

				slashes++;
				dots = 0;
				
				break;
			default:
				dots = 0;
				slashes = 0;
				dst[dst_pos] = path[path_pos];
				break;
		}
	}
	return 0;
}

static struct FSDriver* fs_find_fs(const char* path)
{
	int match_len = 0;
	struct FSDriver* fs = NULL;
	int x;
	for(x = 0;x < FS_TABLE_MAX;x++)
	{
		/* Skip all invalid filesystems */
		if(fstable[x].valid == 0) continue;

		/* Get the length of the mount point of this fs */
		int mp_strlen = strlen(fstable[x].mount_point);

		/* If this ends in a slash, and it's not root, subtract one */
		if(mp_strlen > 1 &&fstable[x].mount_point[mp_strlen - 1]=='/')
			mp_strlen--;

		if(!strncmp(fstable[x].mount_point, (char*)path, mp_strlen))
		{
			/* Find the longest match */
			if(match_len < mp_strlen)
                        {
				match_len = mp_strlen;
				fs = fstable + x;
			}

			continue;
		}
	}

	return fs;
}

static int fs_get_path(struct FSDriver* fs, const char* path, 
		char* dst, size_t sz)
{
	char* mount = fs->mount_point;

	/* Check to see if we're looking for the root */
	if(strlen(mount) - 1 == strlen(path)
		&& mount[strlen(mount - 1)] == '/')
	{
		strncpy(dst, "/", sz);
		return 0;
	}

	int pos = strlen(mount);
	if(pos > strlen(path)) return -1;
	/* We want to keep the trailing slash. */
	strncpy(dst, path + pos - 1, sz);
	return 0;
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

#ifdef CACHE_PANIC
	if(!result) panic("Out of fsman inodes!\n");
#endif

	return result;
}

/**
 * Free an inode that is no longer in use. 
 */
static void fs_free_inode(inode i)
{
#ifdef CACHE_WATCH
        if(rproc) cprintf("INODE FREED: %s process: %s pid: %d\n",
                                i->name, rproc->name, rproc->pid);
        else cprintf("INODE FREED: %s process: KERNEL\n",
                                i->name);
#endif
	memset(i, 0, sizeof(struct inode_t));
}

static int fs_get_name(const char* path, char* dst, size_t sz)
{
	int index = strlen(path);
	for(;index >= 0 && path[index] != '/';index--);
	index++;
	if(index < 0) return 1; /* invalid path */
	strncpy(dst, path + index, sz);
	return 0; 
}

static void fs_sync_inode(inode i)
{
	/* Get a stat structure */
	struct stat s;
	i->fs->stat(i->inode_ptr, &s, i->fs->context);
	memmove(&i->st, &s, sizeof(struct stat));
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

inode fs_open(const char* path, int flags, mode_t permissions,
	uid_t uid, gid_t gid)
{

#ifdef DEBUG
	cprintf("fsman: opening %s\n", path);
#endif
	/* We need to use the driver function for this. */
	char dst_path[FILE_MAX_PATH];
	char tmp_path[FILE_MAX_PATH];
	memset(dst_path, 0, FILE_MAX_PATH);
	memset(tmp_path, 0, FILE_MAX_PATH);
	if(fs_path_resolve(path, tmp_path, FILE_MAX_PATH))
		return NULL; /* Invalid path */
	strncpy(dst_path, tmp_path, FILE_MAX_PATH);
	if(file_path_file(dst_path))
		return NULL;

#ifdef DEBUG
	cprintf("fsman: after resolving file: %s\n", dst_path);
#endif

	/* Find the file system for this path */
	struct FSDriver* fs = fs_find_fs(dst_path);
	if(fs == NULL)
		return NULL; /* Invalid path */

#ifdef DEBUG
	cprintf("fsman: file lives on device mounted at %s\n",
		fs->mount_point);
#endif

        char fs_path[FILE_MAX_PATH];
        if(fs_get_path(fs, dst_path, fs_path, FILE_MAX_PATH))
                return NULL; /* Bad path */

#ifdef DEBUG
	cprintf("fsman: Final resolved path for file: %s\n",
		fs_path);
#endif

	/* See if this file is already opened. */
	void* inp = NULL;

	if((inp = fs->opened(fs_path, fs->context)))
	{
		/* Find the file in our cache. */
		int x;
		for(x = 0;x < FS_INODE_MAX;x++)
		{
			if(itable[x].valid && itable[x].fs == fs
				&& itable[x].inode_ptr == inp)
			{
				/* its already opened. */
				fs_add_inode_reference(itable + x);
				return itable + x;
			}
		}

		/**
		 * If we got here, its open in the FSDriver but we
		 * don't have any record of it in our cache.
		 */
	}

	/* Try creating the file first */
	if(flags & O_CREAT && !inp)
		if(fs->create(fs_path, permissions, uid, gid, fs->context))
			return NULL; /* Permission denied */

	struct inode_t* i = fs_find_inode();
	if(i == NULL)
		return NULL; /* No more inodes are available */
	i->fs = fs;

	if(!inp) inp = fs->open(fs_path, fs->context);
	i->inode_ptr = inp;

	if(inp == NULL)
	{
#ifdef CACHE_WATCH
		if(rproc) cprintf("INODE ACCESS DENIED: %s process: %s pid: %d\n",
				i->name, rproc->name, rproc->pid);
		else cprintf("INODE ACCESS DENIED: %s process: KERNEL\n",
				i->name);
#endif
		fs_free_inode(i);
		return NULL; /* Permission denied */
	}	

	/* Lets quickly get the stats on the file */
	struct stat st;
	fs->stat(i->inode_ptr, &st, fs->context);

	i->file_pos = 0;
	memmove(&i->st, &st, sizeof(struct stat));
	i->references = 1;
	fs_get_name(dst_path, i->name, FILE_MAX_NAME);

#ifdef CACHE_WATCH
	if(rproc) cprintf("FILE OPENED: %s process: %s pid: %d\n",
			i->name, rproc->name, rproc->pid);
	else cprintf("FILE OPENED: %s process: KERNEL\n",
			i->name);
#endif

	return i;
}

int fs_close(inode i)
{
	/* Close the file system file */
	i->references--;	
	if(i->references == 0)
	{ 
		int close_result = i->fs->close(i->inode_ptr, 
				i->fs->context);
		fs_free_inode(i);
		return close_result;
	} else {
#ifdef CACHE_WATCH
		if(rproc) cprintf("INODE DEREFERENCED: %s process: %s pid: %d\n",
				i->name, rproc->name, rproc->pid);
		else cprintf("INODE DEREFERENCED: %s process: KERNEL\n",
				i->name);
#endif
	}
	return 0;
}

int fs_stat(inode i, struct stat* dst)
{
	/* Return the cached version */
	memmove(dst, &i->st, sizeof(struct stat));
	return 0;
}

int fs_create(const char* path, int flags, 
		mode_t permissions, uid_t uid, gid_t gid)
{
	/* fix up the path*/
	char dst_path[FILE_MAX_PATH];
	char tmp_path[FILE_MAX_PATH];
	memset(dst_path, 0, FILE_MAX_PATH);
	memset(tmp_path, 0, FILE_MAX_PATH);
	if(fs_path_resolve(path, tmp_path, FILE_MAX_PATH))
		return -1; /* bad path */
	strncpy(dst_path, tmp_path, FILE_MAX_PATH);
	if(file_path_file(dst_path))
		return -1;

	/* Find the file system this file belongs to */
	struct FSDriver* fs = fs_find_fs(dst_path);
	if(fs == NULL) return -1; /* invalid path */	

	char fs_path[FILE_MAX_PATH];
	if(fs_get_path(fs, dst_path, fs_path, FILE_MAX_PATH))
		return -1;

	/* Create the file with the driver */
	if(fs->create(fs_path, 
				permissions, uid, gid, fs->context))
		return -1;
	return 0;
}

int fs_chown(const char* path, uid_t uid, gid_t gid)
{
	inode i = fs_open(path, O_RDWR, 0x0, 0x0, 0x0);
	if(!i) return -1;

	int result = i->fs->chown(i->inode_ptr,
			uid, gid, i->fs->context);

	/* Close the inode */
	fs_close(i);

	return result;
}

int fs_chmod(const char* path, ushort permission)
{
	inode i = fs_open(path, O_RDWR, 0x0, 0x0, 0x0);
	if(!i) return -1;
	/* Dont allow the type to change */
	permission &= ~S_IFMT;

	/* This is file system specific */
	int result = i->fs->chmod(i->inode_ptr, 
			permission, i->fs->context);

	/* Close the inode */
	fs_close(i);

	return result;
}

int fs_sync(void)
{
	int x;
	for(x = 0;x < FS_TABLE_MAX;x++)
	{
		if(fstable[x].valid)
		{
			if(fstable[x].sync)
				fstable[x].sync(fstable[x].context);
		}
	}

	return 0;
}

int fs_truncate(inode i, int sz)
{
	int result = i->fs->truncate(i->inode_ptr, 
			sz, i->fs->context);
	if(result < 0) return -1;
	else i->st.st_size = result;
	return result;
}

int fs_link(const char* oldpath, const char* newpath)
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

int fs_mkdir(const char* path, int flags, 
		mode_t permissions, uid_t uid, gid_t gid)
{
	/* First resolve the path */
	char dst_path[FILE_MAX_PATH];
	int result = fs_path_resolve(path, dst_path, FILE_MAX_PATH);
	if(result)
		return -1; /* Bad path */

	/* Find the file system */
	struct FSDriver* fs = fs_find_fs(dst_path);

	char fs_path[FILE_MAX_PATH];
	if(fs_get_path(fs, dst_path, fs_path, FILE_MAX_PATH))
		return -1;

	/* Try to create the directory */
	if(fs->mkdir(fs_path, permissions, uid, gid, fs->context))
		return -1;
	return 0;
}

int fs_read(inode i, void* dst, size_t sz, fileoff_t start)
{
	int bytes = i->fs->read(i->inode_ptr, dst, start, sz, i->fs->context);
	/* Check for read error */
	if(bytes < 0) return -1;
	return bytes;
}

int fs_write(inode i, void* src, size_t sz, fileoff_t start)
{
	int bytes = i->fs->write(i->inode_ptr,
			src, start, sz, i->fs->context);
	/* Check for read error */
	if(bytes < 0) return -1;

	/* Update our file position */
	i->file_pos += bytes;

	/* The file properties may have changed */
	fs_sync_inode(i);

	/* TODO: Temporary: write disk after every write */
	// fs_sync();

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

	char fs_src[FILE_MAX_PATH];
	char fs_dst[FILE_MAX_PATH];
	if(fs_get_path(src_fs, src_resolved, fs_src, FILE_MAX_PATH))
		return -1; /* bad fs mount point path */
	if(fs_get_path(dst_fs, dst_resolved, fs_dst, FILE_MAX_PATH))
		return -1; /* bad fs mount point path */

	/**
	 * If the two file systems are not equal, we can't just update
	 * the pointer. We need to create a new file and move the data.
	 */

	int result;

	if(src_fs != dst_fs)
	{
		result = -1;
	} else {
		result = src_fs->rename(fs_src, fs_dst, src_fs->context);
	}

	return result;
}

int fs_unlink(const char* file)
{
	/* Resolve the file path */
	char file_resolved[FILE_MAX_PATH];
	char file_tmp[FILE_MAX_PATH];
	if(fs_path_resolve(file, file_resolved, FILE_MAX_PATH))
		return -1;
	strncpy(file_tmp, file_resolved, FILE_MAX_PATH);
	if(file_path_file(file_tmp))
		return -1;

	struct FSDriver* fs = fs_find_fs(file_tmp);
	if(fs_get_path(fs, file_tmp, file_resolved, FILE_MAX_PATH))
		return -1;
	int result = fs->unlink(file_resolved, fs->context);

	return result;
}

int fs_readdir(inode i, int index, struct dirent* dst)
{
	return i->fs->readdir(i->inode_ptr, index, dst, i->fs->context);
}

int fs_mount(const char* device, const char* point)
{
	return 0;	
}


void fs_fsstat(void)
{
	/*
	   struct fs_stat fss;
	   int x;
	   for(x = 0;x < FS_TABLE_MAX;x++)
	   {
	   struct FSDriver* fs = fstable + x;
	   if(!fs->valid) continue;

	   fs->fsstat(&fss, &fs->context);
	   tty_print_string(rproc->t, "Mount point: %s\n", 
	   fs->mount_point);
	   tty_print_string(rproc->t, 
	   "Free cache inodes: %d\n", fss.cache_free);
	   tty_print_string(rproc->t, 
	   "Allocated cache inodes: %d\n", fss.cache_allocated);
	   tty_print_string(rproc->t, 
	   "Free disk inodes: %d\n", fss.inodes_available);
	   tty_print_string(rproc->t, 
	   "Allocated disk inodes: %d\n", fss.inodes_allocated);
	   tty_print_string(rproc->t, 
	   "Free disk blocks: %d\n", fss.blocks_available);
	   tty_print_string(rproc->t, 
	   "Allocated disk blocks: %d\n", fss.blocks_allocated);
	   if(fss.blocks_allocated == 0 ||
	   fss.blocks_available == 0) continue;
	   tty_print_string(rproc->t, "Free space:      %dK\n", 
	   (fss.blocks_available / 2));
	   tty_print_string(rproc->t, "Allocated space: %dK\n", 
	   (fss.blocks_allocated / 2));
	   tty_print_string(rproc->t, "\n");
	   }
	 */
}

int fs_rmdir(const char* path)
{
	/* Resolve the path */
	char res_path[FILE_MAX_PATH];
	if(fs_path_resolve(path, res_path, FILE_MAX_PATH))
		return -1; /* Bad path */

	/* Get the file system for the path */
	struct FSDriver* fs = fs_find_fs(res_path);
	if(!fs) return -1; /* Bad path */

	/* Get the file system version of the path */
	char fs_path[FILE_MAX_PATH];
	if(fs_get_path(fs, res_path, fs_path, FILE_MAX_PATH))
		return -1; /* Path is not in this file system */
	if(file_path_file(fs_path))
		return -1;

	/* remove the directory */
	if(fs->rmdir(fs_path, fs->context))
		return -1;
	return 0;
}

int fs_mknod(const char* path, dev_t dev, dev_t dev_type, mode_t perm)
{
	char res_path[FILE_MAX_PATH];
	if(fs_path_resolve(path, res_path, FILE_MAX_PATH))
		return -1; /* Bad path */

	/* Get the file system for the path */
	struct FSDriver* fs = fs_find_fs(res_path);
	if(!fs) return -1; /* Bad path */

	/* Get the file system path */
	char fs_path[FILE_MAX_PATH];
	if(fs_get_path(fs, res_path, fs_path, FILE_MAX_PATH))
		return -1;

	/* Make sure the path doesn't end with a slash */
	if(file_path_file(fs_path))
		return -1;

	/* Create the node if it's supported */
	if(fs->mknod && fs->mknod(fs_path, dev, dev_type, perm, fs->context))
		return -1;
	return 0;
}

int fs_pathconf(int config, const char* path)
{
    inode file = fs_open(path, O_RDONLY, 0000, 0x0, 0x0);
	if(!file) 
	{
#ifdef DEBUG
		cprintf("pathconf: file doesn't exist.\n");
#endif
		return -1;
	}

	if(!file->fs->pathconf)
	{
#ifdef DEBUG
		cprintf("pathconf: file lives on system with no pathconf.\n");
#endif
		return -1;
	}

    int result = -1;
    switch(config)
    {
        case _PC_PIPE_BUF:
			return 8192;
		case _PC_VDISABLE: /* FIXME: this should call the driver's pathconf */
			return 0;
		case _PC_MAX_CANON: /* FIXME: this should call the driver's pathconf */
		case _PC_MAX_INPUT:
			return 256;
        case _PC_NO_TRUNC:
        case _PC_LINK_MAX:
        case _PC_NAME_MAX:
        case _PC_PATH_MAX:
        case _PC_CHOWN_RESTRICTED:
		default:
			result = file->fs->pathconf(config, path, file->fs);
			break;
    }

    return result;
}

int fs_readline(inode i, size_t curr_offset, void* buffer, size_t sz)
{
	char* buff_c = buffer;
	/* fill the buffer */
	int result = fs_read(i, buff_c, sz - 1, curr_offset);

	if(result > sz - 1)
		panic("kernel: file system wrote too much!!\n");

	/* Check for read error */
	if(result < 0) return -1;

	/* Check for EOF */
	if(result == 0)
	{
		*buff_c = 0;
		return 0;
	}

	/* Find first newline (if it exists) */
	int x;
	for(x = 0;x < result - 1;x++)
	{
		if(!buff_c[x]) break;
		if(buff_c[x] == '\n')
		{
			x++;
			break;
		}
	}

	/* put null character into the buffer */
	buff_c[x] = 0;

	return x;
}
