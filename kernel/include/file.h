#ifndef _FILE_H_
#define _FILE_H_

#include <sys/types.h>

#undef FILE_MAX_PATH
#define FILE_MAX_PATH	256
#undef FILE_MAX_NAME
#define FILE_MAX_NAME	256

/**
 * For use in functions requiring permissions:
 */

#define PERM_AAP 0777	/* Everyone has all permissions */
#define PERM_ARD 0444 	/* Everyone can read */
#define PERM_AWR 0222 	/* Everyone can write*/
#define PERM_AEX 0111 	/* Everyone can execute */

#define PERM_UAP 0700	/* User has all permissions */
#define PERM_URD 0400 	/* User can read */
#define PERM_UWR 0200 	/* User can write */
#define PERM_UEX 0100 	/* User can execute */

#define PERM_GAP 0070	/* Group has all permissions */
#define PERM_GRD 0040 	/* Group can read */
#define PERM_GWR 0020 	/* Group can write */
#define PERM_GEX 0010 	/* Group can execute */

#define PERM_OAP 0007	/* Others have all permissions */
#define PERM_ORD 0004 	/* Others can read */
#define PERM_OWR 0002 	/* Others can write */
#define PERM_OEX 0001 	/* Others can execute */

#define S_ISDEV(m) ((S_ISBLK(m)) || (S_ISCHR(m)) || (S_ISFIFO(m)))

/* Device nodes */
struct devnode
{
        int dev;
        int type;
};

/* Linux Permission Macros */
#ifndef __LINUX__

//struct chronos_dirent {
//        ino_t d_ino; /* inode number */
//        off_t d_off; /* next directory index number */
//        unsigned short d_reclen; /* The length of this structure */
//        unsigned short d_type; /* File type (fs specific) */
///        char name[FILE_MAX_NAME]; /* The name of the directory */
//};

// #define S_IRWXU 00700 /* User has read, write execute */
// #define S_IRUSR 00400 /* User has read, write execute */
// #define S_IWUSR 00200 /* User has read, write execute */
// #define S_IXUSR 00100 /* User has read, write execute */

// #define S_IRWXG 00070 /* Group has read, write execute */
// #define S_IRGRP 00040 /* Group has read, write execute */
// #define S_IWGRP 00020 /* Group has read, write execute */
// #define S_IXGRP 00010 /* Group has read, write execute */

// #define S_IRWXO 00007 /* Other has read, write execute */
// #define S_IROTH 00004 /* Other has read, write execute */
// #define S_IWOTH 00002 /* Other has read, write execute */
// #define S_IXOTH 00001 /* Other has read, write execute */

/* Special Linux permissions */

// #define S_IFMT   0170000 /* Determine file type */
// #define S_IFDIR  0040000 /* Directory */
// #define S_IFCHR  0020000 /* Character device */
// #define S_IFBLK  0060000 /* Block device */
// #define S_IFREG  0100000 /* Regular file */
// #define S_IFIFO  0010000 /* FIFO device */
// #define S_IFLNK  0120000 /* Symbolic link */
// #define S_IFSOCK 0140000 /* Socket*/

// #define S_ISUID	0004000 /* Set user id bit*/
// #define S_ISGID	0002000 /* Set group id bit */
// #define S_ISVTX	0001000 /* Sticky bit */

/* Linux file macros */

// #define S_ISBLK(m)   (((m) & S_IFMT) == S_IFBLK)
// #define S_ISCHR(m)   (((m) & S_IFMT) == S_IFCHR)
// #define S_ISDIR(m)   (((m) & S_IFMT) == S_IFDIR)
// #define S_ISFIFO(m)  (((m) & S_IFMT) == S_IFIFO)
// #define S_ISREG(m)   (((m) & S_IFMT) == S_IFREG)
// #define S_ISLNK(m)   (((m) & S_IFMT) == S_IFLNK)
// #define S_ISSOCK(m)  (((m) & S_IFMT) == S_IFSOCK)

/* POSIX.1b objects UNUSED - compatibility */
#define S_TYPEISMQ(buf)  ((buf)->st_mode - (buf)->st_mode)
#define S_TYPEISSEM(buf) ((buf)->st_mode - (buf)->st_mode)
#define S_TYPEISSHM(buf) ((buf)->st_mode - (buf)->st_mode)

/**
 * Structure for getting statistics on a file.
 */
// struct stat
// {
//	dev_t	st_dev; /* Device ID */
//	ino_t	st_ino; /* Inode number */
//	mode_t	st_mode; /* Protection (permission) of the file */
//	nlink_t	st_nlink; /* Number of hard links to the file */
//	uid_t	st_uid; /* User ID of owner */
//	gid_t	st_gid; /* Group ID of owner */
//	dev_t	st_rdev; /* Device id if special file */
//	off_t	st_size; /* Size of the file in bytes */
//	blksize_t st_blksize; /* Blocksize for the file IO */
//	blkcnt_t st_blocks; /* Number of 512B blocks */
//	time_t st_atime; /* Time of last access */
//	time_t st_mtime; /* Last modification */
//	time_t st_ctime; /* Last time metadata was changed */
//};

/**
 * Represents a generic directory entry.
 */
//struct dirent
//{
//	ino_t d_ino; /* Inode number of the directory entry */
//	off_t d_off; /* Offset to the next dirent */
//	unsigned short d_reclen; /* Length of this record */
//	unsigned char  d_type; /* Type of file (see above) */
//	char d_name[FILE_MAX_NAME]; /* name of the directory entry */
//};

/**
 * Older version of the linux directory entry structure.
 */
//struct old_linux_dirent
//{
//	long d_ino; /* inode number*/
//	off_t d_off; /* Offset into the directory file */
//	ushort d_reclen; /* Max size of d_name */
//	char d_name[FILE_MAX_NAME]; /* Name of the entry */
//};

#endif

#ifndef __FILE_NO_FUNC__
/**
 * Make sure the path ends with a slash. This guarentees that the path
 * represents a directory. Returns -1 on failure.
 */
int file_path_dir(char* path, size_t sz);

/**
 * Makes sure that the path doesn't end with a slash unless src is the root
 * directory. Returns 0 on success.
 */
int file_path_file(char* file);

/**
 * Returns the parent of src into dst. A maximum of sz bytes will be copied
 * into the destination buffer dst. Does NOT work on relative paths.
 * Returns 0 on success, returns 1 if src has no parent.
 */
int file_path_parent(char* path);

/**
 * Find the file's name that this path represents. This also accounts for
 * paths that end in slashes. May not work on relative paths. A maximum
 * of sz bytes are copied into the dst buffer. Always returns 0.
 */
int file_path_name(char* file);

/**
 * Puts the root of the path into dst. A maximum of sz bytes will be
 * copied. Returns 0 on success, returns -1 if there is no parent 
 * (e.g. the source is "").
 */
int file_path_root(char* file);

/**
 * Remove the first prefix from the path. This will remove a 
 * slash (optional) and then remove characters until another
 * slash is encountered. Returns the position of the resulting
 * string.
 */
char* file_remove_prefix(const char* path);

#endif

#endif
