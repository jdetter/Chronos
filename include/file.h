#ifndef _FILE_H_
#define _FILE_H_

#define FILE_MAX_PATH	256
#define FILE_MAX_NAME	64

#define FILE_TYPE_FILE    0x01
#define FILE_TYPE_DIR     0x02
#define FILE_TYPE_DEVICE  0x03
#define FILE_TYPE_SPECIAL 0x04



/**
 * For use in function fs_seek:
 *  + SEEK_SET Sets the current seek from the start of the file.
 *  + SEEK_CUR Adds the value to the current seek
 *  + SEEK_END Sets the current seek from the end of the file.
 */
#define SEEK_SET 0x01
#define SEEK_END 0x02
#define SEEK_CUR 0x03

#define PERM_ARD 0444 /* Everyone can read */
#define PERM_AWR 0222 /* Everyone can write*/
#define PERM_AEX 0111 /* Everyone can execute */

#define PERM_URD 0400 /* User can read */
#define PERM_UWR 0200 /* User can write */
#define PERM_UEX 0100 /* User can execute */

#define PERM_GRD 040 /* Group can read */
#define PERM_GWR 020 /* Group can write */
#define PERM_GEX 010 /* Group can execute */

#define PERM_ORD 04 /* Others can read */
#define PERM_OWR 02 /* Others can write */
#define PERM_OEX 01 /* Others can execute */

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

#endif
