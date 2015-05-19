#ifndef _FILE_H_
#define _FILE_H_

/**
 * A user friendly statistic structure on a file.
 */
typedef struct file_stat_struct
{
	int type; /* The type of the file*/
	short perm; /* Owner: RWX Group: RWX Other: RWX*/
	short uid; /* Owner ID */
	short gid; /* Group ID */
	short links_count; /* Hard links */
	uint size; /* How many bytes are in the file */
} file_stat;

#endif
