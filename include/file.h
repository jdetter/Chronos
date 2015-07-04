#ifndef _FILE_H_
#define _FILE_H_

#define FILE_MAX_PATH	256
#define FILE_MAX_NAME	64

#define FILE_TYPE_FILE    0x01
#define FILE_TYPE_DIR     0x02
#define FILE_TYPE_DEVICE  0x03
#define FILE_TYPE_LINK 	  0x04
#define FILE_TYPE_SPECIAL 0x05

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

/**
 * Flag options for use in open:
 */
#define O_CREATE  0x01	/* If the file has not been created, create it. */
#define O_APPEND  0x02	/* Start writing to the end of the file. */
#define O_DIR     0x04	/* Fail to open the file unless it is a directory. */
#define O_NOATIME 0x08 	/* Do not change access time when file is opened. */
#define O_TRUC    0x10	/* Once the file is opened, truncate the contents. */
#define O_SERASE  0x20  /* Securely erase files when deleting or truncating */
#define O_RDONLY  0x40 /* Open for reading */
#define O_WRONLY  0x80 /* Open for writing */
#define O_RDWR	  (O_RDONLY | O_WRONLY)

/**
 * Structure for getting statistics on a file.
 */
struct file_stat
{
        uint owner_id; /* Owner of the file */
        uint group_id; /* Group that owns the file */
        uint perm; /* Permissions of the file */
        uint sz; /* Size of the file in bytes */
        uint inode; /* inode number of the file  */
        uint links; /* The amount of hard links to this file */
        uint type; /* See options for file type above*/
};

/**
 * Represents a generic directory entry.
 */
struct directent
{
	uint inode; /* Inode number of the directory entry */
	uint type; /* Type of file (see above) */
	char name[FILE_MAX_NAME]; /* name of the directory entry */
};

/* Device nodes */
struct devnode
{
        uint dev;
        uint type;
};

/**
 * Make sure the path ends with a slash. This guarentees that the path
 * represents a directory. Returns -1 on failure.
 */
int file_path_dir(const char* src, char* dst, uint sz);

/**
 * Makes sure that the path doesn't end with a slash unless src is the root
 * directory. Returns 0 on success.
 */
int file_path_file(const char* src, char* dst, uint sz);

/**
 * Returns the parent of src into dst. A maximum of sz bytes will be copied
 * into the destination buffer dst. Does NOT work on relative paths.
 * Returns 0 on success, returns 1 if src has no parent.
 */
int file_path_parent(const char* src, char* dst, uint sz);

/**
 * Find the file's name that this path represents. This also accounts for
 * paths that end in slashes. May not work on relative paths. A maximum
 * of sz bytes are copied into the dst buffer. Always returns 0.
 */
int file_path_name(const char* src, char* dst, uint sz);

#endif
