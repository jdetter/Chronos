#ifndef _FSMAN_H_
#define _FSMAN_H_

#define FS_HARDWARE_CONTEXT_SIZE 64

struct FSHardwareDriver
{
	uchar valid; /* 1 = valid, 0 = invalid. */
	int (*read)(void* dst, uint sect, 
		struct FSHardwareDriver* driver); 
	int (*write)(void* src, uint sect, 
		struct FSHardwareDriver* driver);
	void* context; /* Pointer to driver specified context */
};

/**
 * Needed functions in the driver interface:
 * seek: change position in file
 * open: open and possibly create a file
 * close: close an open file
 * create: create a file
 * chown: change ownership of file
 * chmod: change permissions of file
 * stat: get stat on file
 * sync: flush file to disk
 * truncate: truncate a file to a length
 * link: create a soft link
 * mkdir: create a directory
 * mknod: make a device node
 * read: read from a file
 * readdir: read a directory entry
 * rename: move a file
 * rmdir: remove a directory
 * unlink: delete a file reference
 * mount: create a new file system instance
 * unmount: close file system instance
 */

#define FS_INODE_MAX 256 /* Number of inodes in the inode table */
#define FS_TABLE_MAX 4 /* Maximum number of mounted file systems */
#define FS_CACHE_SIZE 16384
#define FS_CONTEXT_SIZE 128 /* Size in bytes of an fs context */

/** 
 * General inode that is used by the operating system. The
 * file system driver worries about the rest.
 */
struct inode_t
{
	uchar valid; /* Whether or not this inode is valid. */
	slock_t lock; /* Lock needs to be held to access this inode */
	uint file_pos; /* Seek position in the file */
	struct stat st; /* Stats on the file */
	char name[FILE_MAX_NAME]; /* the name of the file */
	struct FSDriver* fs; /* File system this inode belongs to.*/
	void* inode_ptr; /* Pointer to the fs specific inode. */
	int references; /* How many programs reference this file? */
};
typedef struct inode_t* inode;

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
	 * Initilize the hardware and context needed for the file system
	 * to function.
	 */
	int (*init)(uint start_sector, uint end_sector, uint block_size,
		uint cache_sz, uchar* cache, void* context);
	
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
	int (*stat)(void* i, struct stat* dst,
			void* context);

	/**
 	 * Create a file in the file system given by path. Returns 0 on
	 * success, -1 on failure. If the file already exists, returns 0.
	 */
	int (*create)(const char* path, uint permissions, uint uid,
		uint gid, void* context);

	/**
	 * Change the ownership of the file. Both the uid and
	 * the gid will be changed. Returns 0 on success.
	 */
	int (*chown)(void* i, int ino_num, 
		uint uid, uint gid, void* context);

	/**
	 * Change the permissions on a file. The bits for the permission
	 * field are defined in file.h. Returns the new permission
	 * of the file. 
	 */
	int (*chmod)(void* i, int ino_num, 
		uint permission, void* context);

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
	int (*mkdir)(const char* path, uint permissions, uint uid, uint gid, 
		void* context);

	/**
	 * Read from the inode i into the buffer dst for sz bytes starting
	 * at position start in the file. Returns the amount of bytes read
	 * from the file.
	 */
	int (*read)(void* i, void* dst, uint start, uint sz, void* context);

        /**
         * Read from the inode i into the buffer dst for sz bytes starting
         * at position start in the file. Returns the amount of bytes read
         * from the file.
         */ 
        int (*write)(void* i, void* dst, uint start, uint sz, void* context);

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
	 * Parse statistics into the stat structure.
	 */
	int (*fsstat)(struct fs_stat* dst, void* context);

	/**
	 * See if the file is already opened. If it is, returns
	 * a pointer to that inode in the cache. If it isn't in
	 * the cache, NULL is returned.
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
	int (*mknod)(const char* path, uint dev, uint dev_type,
                uint perm, void* context);

	/* Locals for the driver */
	uchar valid; /* Whether or not this entry is valid. */
	uint type; /* Type of file system */
	struct FSHardwareDriver* driver; /* HDriver for this file system*/
	char mount_point[FILE_MAX_PATH]; /* Mounted directory */
	uchar context[FS_CONTEXT_SIZE]; /* Space for locals */
	uchar cache[FS_CACHE_SIZE]; /* Space for caching files */
};

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
inode fs_open(const char* path, uint flags, 
		uint permissions, uint uid, uint gid);

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
int fs_create(const char* path, uint flags, 
		uint permissions, uint uid, uint gid);

/**
 * Change the ownership of the file to uid and gid. Returns 0
 * on success, returns -1 on failure.
 */
int fs_chown(const char* path, uint uid, uint gid);

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
 * Create the directory designated by path. Returns an inode describing the
 * directory on success, returns NULL on failure.
 */
inode fs_mkdir(const char* path, uint flags, 
		uint permissions, uint uid, uint gid);

/**
 * Read sz bytes from inode i into the destination buffer dst starting at the
 * seek position start.
 */
int fs_read(inode i, void* dst, uint sz, uint start);

/**
 * Write sz bytes from inode i from the source buffer src into the file at 
 * the seek position start.
 */
int fs_write(inode i, void* src, uint sz, uint start);

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
 * Read the directory entry at the specified index.
 */
int fs_readdir(inode i, int index, struct dirent* dst);

/**
 * Simplify the given path so that it doesn't contain any . or ..s
 * also check to make sure that the path is valid. Returns 1 if the
 * path has grammar errors. Returns 0 on success.
 */
int fs_path_resolve(const char* path, char* dst, uint sz);

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
int fs_mknod(const char* path, int dev, int dev_type, int perm);

/**
 * Truncate a file to a specific length. Returns 0 on success.
 */
int fs_truncate(inode i, int sz);

#endif
