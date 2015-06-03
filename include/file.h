#ifndef _FILE_H_
#define _FILE_H_

#define FTYPE_FILE      0x01
#define FTYPE_DIR       0x02
#define FTYPE_SOFT      0x03
#define FTYPE_DEVICE    0x04

/**
 * Structure for getting statistics on a file.
 */
struct stat
{
        uint owner_id; /* Owner of the file */
        uint group_id; /* Group that owns the file */
        uint perm; /* Permissions of the file */
        uint sz; /* Size of the file in bytes */
        uint links; /* The amount of hard links to this file */
        uint type; /* See options for file type above*/
};

#endif
