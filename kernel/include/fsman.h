#ifndef _FSMAN_H_
#define _FSMAN_H_

#define FS_HARDWARE_CONTEXT_SIZE 64

/* Some dependant headers */
#include <sys/stat.h>
#include <stdint.h>
#include <dirent.h>

#include "file.h"
#include "stdlock.h"
#include "cache.h"
#include "devman.h"

#define FS_INODE_CACHE_SZ 0x10000 /* Per file system cache */

#define FS_INODE_MAX 256 /* Number of inodes in the inode table */
#define FS_TABLE_MAX 4 /* Maximum number of mounted file systems */
#define FS_MAX_INODE_CACHE 256 /* Maximum number of entries */
#define FS_CONTEXT_SIZE 512 /* Size in bytes of an fs context */

/** 
 * General inode that is used by the operating system. The
 * file system driver worries about the rest.
 */
struct inode_t
{
	int valid; /* Whether or not this inode is valid. */
	int flags; /* The flags that were used to open this file */
	slock_t lock; /* Lock needs to be held to access this inode */
	int file_pos; /* Seek position in the file */
	struct stat st; /* Stats on the file */
	char name[FILE_MAX_NAME]; /* the name of the file */
	struct FSDriver* fs; /* File system this inode belongs to.*/
	void* inode_ptr; /* Pointer to the fs specific inode. */
	int references; /* How many programs reference this file? */
};

/**
 * Turn the typedef of inode into a macro so that inode can be
 * typdef'd in other files.
 */
#ifndef NO_DEFINE_INODE
typedef struct inode_t* inode;
#else /* !define inode */
#define inode struct inode_t
#endif

struct fs_stat
{
	int inodes_available;
	int inodes_allocated;
	int blocks_available;
	int blocks_allocated;
	int cache_allocated;
	int cache_free;
};

struct FSDriver
{
	/**
	 * Required file system driver function that opens a file
	 * given a path, some flags and an open mode. The mode and
	 * flags are non-file system specific and are defined in 
	 * file.h. Returns a file system specific pointer to an
	 * inode.
	 */
	void* (*open)(const char* path, void* context);

	/**
	 * Close the file with the given inode. The inode type
	 * is file system specific.
	 */
	int (*close)(void* i, void* context);

	/**
	 * Parse statistics on an open file. Returns 0 on success,
	 * returns 1 otherwise.
	 */
	int (*stat)(void* i, struct stat* dst, void* context);

	/**
	 * Create a file in the file system given by path. Returns 0 on
	 * success, -1 on failure. If the file already exists, returns 0.
	 */
	int (*create)(const char* path, mode_t permissions, 
			gid_t uid, gid_t gid, void* context);

	/**
	 * Change the ownership of the file. Both the uid and
	 * the gid will be changed. Returns 0 on success.
	 */
	int (*chown)(void* i, uid_t uid, gid_t gid, void* context);

	/**
	 * Change the permissions on a file. The bits for the permission
	 * field are defined in file.h. Returns the new permission
	 * of the file. 
	 */
	int (*chmod)(void* i, mode_t permission, void* context);

	/**
	 * Truncate a file to the specified length. Returns the
	 * new length of the file or -1 on Failure.
	 */
	int (*truncate)(void* i, int sz, void* context);

	/**
	 * Create a hard link to a file. Returns 0 on success.
	 */
	int (*link)(const char* file, const char* link, void* context);

	/**
	 * Create a soft link to a file. Returns 0 on success.
	 */
	int (*symlink)(const char* file, const char* link, void* context);

	/**
	 * Make a directory in the file system. Returns 0 on success, -1 on
	 * failure.
	 */
	int (*mkdir)(const char* path, mode_t permissions, uid_t uid, gid_t gid, 
			void* context);

	/**
	 * Read from the inode i into the buffer dst for sz bytes starting
	 * at position start in the file. Returns the amount of bytes read
	 * from the file.
	 */
	int (*read)(void* i, void* dst, fileoff_t start, 
			size_t sz, void* context);

	/**
	 * Read from the inode i into the buffer dst for sz bytes starting
	 * at position start in the file. Returns the amount of bytes read
	 * from the file.
	 */ 
	int (*write)(void* i, const void* src, fileoff_t start, 
			size_t sz, void* context);

	/**
	 * Move file from src to dst. This function can also move
	 * directories.
	 */
	int (*rename)(const char* src, const char* dst, void* context);

	/**
	 * Remove the file or directory given by file. Directories will
	 * be removed without warning.
	 */
	int (*unlink)(const char* file, void* context);

	/**
	 * Read the directory entry at index in the directory dir. Returns
	 * 0 on success, -1 when the end of the directory has been reached.
	 */
	int (*readdir)(void* dir, int index, 
			struct dirent* dst, void* context);

	/**
	 * Get directory entries from an inode. Pos is the offset to the
	 * next available entry. Count is the amount of directory entries
	 * to read. Returns the amount of bytes read. Returns -1 on failure
	 * and returns 0 on end of directory.
	 */
	int (*getdents)(void* dir, struct dirent* dst_arr, int count,
			fileoff_t pos, void* context);

	/**
	 * Parse statistics into the stat structure.
	 */
	int (*fsstat)(struct fs_stat* dst, void* context);

	/**
	 * See if the file is already opened. If it is, returns
	 * a pointer to that inode in the cache. If it isn't in
	 * the cache, NULL is returned. DEPRICATED
	 */
	void* (*opened)(const char* path, void* context);

	/**
	 * Removes a directory from the file system. The directory must
	 * be empty. Return 0 on success, -1 otherwise.
	 */
	int (*rmdir)(const char* path, void* context);

	/** 
	 * Create a node on the file system. This allows programs to have
	 * a common interface to access devices.
	 */
	int (*mknod)(const char* path, dev_t major, dev_t minor,
			mode_t perm, void* context);

	/**
	 * Sync all files and data with the storage. This function never
	 * fails.
	 */
	void (*sync)(void* context);

	/**
	 * Sync a single file and its data with the storage. Returns 0
	 * on success.
	 */
	int (*fsync)(void* i, void* context);

	/**
	 * Run FSCK on this file system. Returns 0 if the file system
	 * is ready to mount, nonzero otherwise.
	 */
	int (*fsck)(void* context);

	/**
	 * Check the configuration value for the file located at the
	 * given path. Returns the configuration value, -1 if the
	 * configuration value doesn't exist.
	 */
	int (*pathconf)(int conf, const char* path, void* context);

	/* Locals for the driver */
	int valid; /* Whether or not this entry is valid. */
	int type; /* Type of file system */
	struct StorageDevice* driver; /* The underlying storage device */
	char mount_point[FILE_MAX_PATH]; /* Mounted directory */
	char context[FS_CONTEXT_SIZE]; /* Space for locals */
	char* inode_cache; /* Pointer into the inode cache */
	size_t inode_cache_sz; /* How big is the cache in bytes? */
	int bpp; /* How many fs blocks fit onto a memory page? */
	sect_t fs_start; /* The sector where this file system starts */

	size_t blocksize; /* What is the block size of this fs? */
	int blockshift; /* Turn a block address into an id */

	void* (*reference)(blk_t block, struct FSDriver* driver);
	void* (*addreference)(blk_t block, struct FSDriver* driver);
	int (*dereference)(void* ref, struct FSDriver* driver);
};

/**
 * The location of the device for the root partition on
 * disk.
 */
extern char root_partition[];

/**
 * Detect all disks and file systems.
 */
void fsman_init(void);

/**
 * Add a reference to an inode.
 */
int fs_add_inode_reference(struct inode_t* i);

/**
 * Open the file with the given path and return an inode that describes the
 * file. The permissions argument is only used if the file doesn't exist and
 * can be created. The creation flag also must be passed. All flags
 * are defined in file.h. Returns NULL if the file could not be opened.
 */
inode fs_open(const char* path, int flags, 
		mode_t permissions, uid_t uid, gid_t gid);

/**
 * Close an open file. Returns 0 on success, -1 otherwise.
 */
int fs_close(inode i);

/**
 * Fill a stat structure with the stats for the given inode. Returns
 * 0 on success, returns -1 on failure.
 */
int fs_stat(inode i, struct stat* dst);

/**
 * Create a file with the given permissions. The file will also be
 * opened if it is created. Returns NULL if the file could not be created,
 * otherwise returns an inode.
 */
int fs_create(const char* path, int flags, 
		mode_t permissions, uid_t uid, gid_t gid);


/**
 * Change the ownership of the file to uid and gid. Returns 0
 * on success, returns -1 on failure.
 */
int fs_chown(const char* path, uid_t uid, gid_t gid);

/**
 * Change the permissions of the file. The permissions are
 * defined in file.h. Returns 0 on success, -1 on failure.
 */
int fs_chmod(const char* path, ushort permission);

/**
 * Creates a hard link to another file. Hard links cannot be
 * created accross file systems. 
 */
int fs_link(const char* file, const char* link);

/**
 * Create a symbolic link in the file system.
 */
int fs_symlink(const char* file, const char* link);

/**
 * Create the directory given by path. Returns 0 on success, -1 on failure.
 */
int fs_mkdir(const char* path, int flags, 
		mode_t permissions, uid_t uid, gid_t gid);

/**
 * Read sz bytes from inode i into the destination buffer dst starting at the
 * seek position start.
 */
int fs_read(inode i, void* dst, size_t sz, fileoff_t start);

/**
 * Write sz bytes from inode i from the source buffer src into the file at 
 * the seek position start.
 */
int fs_write(inode i, void* src, size_t sz, fileoff_t start);

/**
 * Rename (or move) a file from src to dst. Returns 0 on success, 
 * returns -1 on failure.
 */
int fs_rename(const char* src, const char* dst);

/**
 * Remove a hard link to a file. If all hard links are unlinked,
 * the file will be removed permanantly.
 */
int fs_unlink(const char* file);

/**
 * Read the directory entry at the specified index. Returns 0 on success,
 * -1 otherwise.
 */
int fs_readdir(inode i, int index, struct dirent* dst);

/**
 * Simplify the given path so that it doesn't contain any . or ..s
 * also check to make sure that the path is valid. Returns 1 if the
 * path has grammar errors. Returns 0 on success.
 */
int fs_path_resolve(const char* path, char* dst, size_t sz);

/**
 * Prints out file system statistics to the tty of the running process.
 */
void fs_fsstat(void);

/**
 * Remove a directory. The directory must be empty in order for
 * it to be removed. Returns 0 on success, -1 otherwise.
 */
int fs_rmdir(const char* path);

/**
 * Create a device node at the specified path.
 */
int fs_mknod(const char* path, dev_t dev, dev_t dev_type, mode_t perm);

/**
 * Truncate a file to a specific length. Returns 0 on success.
 */
int fs_truncate(inode i, int sz);

/**
 * Sync all file system data to the underlying storage device. 
 * Return 0 on success.
 */
int fs_sync(void);

/**
 * Get the configuration value for the given file. Returns the
 * configuration value. If the configuration value doesn't exist,
 * then -1 is returned.
 */
int fs_pathconf(int conf, const char* path);

/**
 * Read a line from the given inode. A line is designated by a newline
 * character. file_position should be updated by the caller when
 * readline returns. dst_buffer must be at least sz bytes in size.
 * Returns the amount of bytes read from the file. The newline
 * character is included in this count AND is included in the buffer.
 * Therefore, a blank link will return 1, which a newline character
 * in the null character terminated buffer.
 */
int fs_readline(inode i, size_t file_position, void* dst_buffer, size_t sz);

/**
 * Get rid of the macro declaration.
 */
#ifdef NO_DEFINE_INODE
#undef inode
#endif

#endif
