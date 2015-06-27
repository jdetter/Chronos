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
 * For use in function fs_seek:
 *  + SEEK_SET Sets the current seek from the start of the file.
 *  + SEEK_CUR Adds the value to the current seek
 *  + SEEK_END Sets the current seek from the end of the file.
 */
#define SEEK_SET 0x01
#define SEEK_END 0x02
#define SEEK_CUR 0x04

/**
 * For use in functions requiring permissions:
 */

#define PERM_ARD 0444 	/* Everyone can read */
#define PERM_AWR 0222 	/* Everyone can write*/
#define PERM_AEX 0111 	/* Everyone can execute */

#define PERM_URD 0400 	/* User can read */
#define PERM_UWR 0200 	/* User can write */
#define PERM_UEX 0100 	/* User can execute */

#define PERM_GRD 040 	/* Group can read */
#define PERM_GWR 020 	/* Group can write */
#define PERM_GEX 010 	/* Group can execute */

#define PERM_ORD 04 	/* Others can read */
#define PERM_OWR 02 	/* Others can write */
#define PERM_OEX 01 	/* Others can execute */

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

#endif
