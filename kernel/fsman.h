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
#define FS_TABLE_MAX 16 /* Maximum number of mounted file systems */
#define FS_CACHE_SIZE 1024
#define FS_CONTEXT_SIZE 128 /* Size in bytes of an fs context */

/** 
 * General inode that is used by the operating system. The
 * file system driver worries about the rest.
 */
struct inode_t
{
	uchar valid; /* Whether or not this inode is valid. */
	uint file_pos; /* Seek position in the file */
	uint file_sz; /* Length of the file in bytes. */
	uchar file_type; /* Type of file */
        uint inode_num; /* inode number */
	char name[FILE_MAX_NAME]; /* the name of the file */
	struct FSDriver* fs; /* File system this inode belongs to.*/
	void* inode_ptr; /* Pointer to the fs specific inode. */
	int references; /* How many programs reference this file? */
};
typedef struct inode_t* inode;

struct FSDriver
{
	/**
	 * Initilize the hardware and context needed for the file system
	 * to function.
	 */
	int (*init)(uint start_sector, uint end_sector, uint block_size,
		void* context);
	
	/**
	 * Required file system driver function that opens a file
	 * given a path, some flags and an open mode. The mode and
	 * flags are non-file system specific and are defined in 
	 * file.h. Returns a file system specific pointer to an
	 * inode.
	 */
	void* (*open)(const char* path, uint flags, uint permissions,
			void* context);

	/**
 	 * Close the file with the given inode. The inode type
	 * is file system specific.
 	 */
	int (*close)(void* i, void* context);

	/**
 	 * Parse statistics on an open file. Returns 0 on success,
	 * returns 1 otherwise.
 	 */
	int (*stat)(void* i, struct file_stat* dst,
			void* context);

	/**
 	 * Create a file in the file system given by path. Flags
	 * and mode are defined in file.h. Returns a pointer to
	 * the cached inode. 
	 */
	void* (*create)(const char* path, uint flags, uint permissions,
			void* context);

	/**
	 * Change the ownership of the file. Both the uid and
	 * the gid will be changed. Returns 0 on success.
	 */
	int (*chown)(void* i, uint uid, uint gid, void* context);

	/**
	 * Change the permissions on a file. The bits for the permission
	 * field are defined in file.h. Returns the new permission
	 * of the file. 
	 */
	int (*chmod)(void* i, uint permission, void* context);

	/**
	 * Truncate a file to the specified length. Returns the
	 * new length of the file or -1 on Failure.
	 */
	int (*truncate)(void* i, int sz, void* context);

	/**
	 * Create a hard link to a file. Returns 0 on success.
	 */
	int (*link)(void* i, const char* file, 
			const char* link, void* context);

        /**
         * Create a soft link to a file. Returns 0 on success.
         */
        int (*symlink)(void* i, const char* file, 
                        const char* link, void* context);

	/**
	 * Make a directory in the file system. Returns an inode that
	 * points to the newly created directory. The permissions and flags
	 * are defined in file.h
	 */
	inode (*mkdir)(const char* path, uint flags, 
		uint permissions, void* context);

	/**
	 * Read from the inode i into the buffer dst for sz bytes starting
	 * at position start in the file. Returns the amount of bytes read
	 * from the file.
	 */
	int (*read)(void* i, void* dst, uint sz, uint start, void* context);

        /**
         * Read from the inode i into the buffer dst for sz bytes starting
         * at position start in the file. Returns the amount of bytes read
         * from the file.
         */ 
        int (*write)(void* i, void* dst, uint sz, uint start, void* context);

	/**
	 * Move file from src to dst. This function can also move
	 * directories.
	 */
	int (*rename)(const char* src, const char* dst, void* context);

	/**
	 * Remove the file or directory given by file. Directories will
	 * be removed without warning.
	 */
	int (*unlink)(const char* file);

	/* Locals for the driver */
	uchar valid; /* Whether or not this entry is valid. */
	uint type; /* Type of file system */
	struct FSHardwareDriver* driver; /* HDriver for this file system*/
	//struct filesystem_driver* driver; /* Driver for the file system*/
	char mount_point[FILE_MAX_PATH]; /* Mounted directory */

	uchar context[FS_CONTEXT_SIZE]; /* Space for locals + cache */
};

/**
 * Detect all disks and file systems.
 */
void fsman_init(void);

/**
 * Seek to a position in a file. The constants for whence are 
 * SEEK_SET, SEEK_END, and SEEK_CUR.
 */
uint fs_seek(inode i, int pos, uchar whence);

/**
 * Get statistics on a file. The information goes into the
 * destination buffer dst.
 */
int fs_fstat(inode i, struct file_stat* dst);

#endif
