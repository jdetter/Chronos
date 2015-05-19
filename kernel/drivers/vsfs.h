#ifndef _VSFS_H_
#define _VSFS_H_

/* File Types */
#define VSFS_FILE 0x0 /* Normal File */
#define VSFS_DIR  0x1 /* Directory */
#define VSFS_DEV  0x2 /* Device node */
#define VSFS_SYM  0x3 /* Symbolic link to another file */

/**
 * A disk representation of an inode.
 */
typedef struct vsfs_inode {
	ushort perm; /* Owner: RWX Group: RWX Other: RWX*/
	ushort uid; /* Owner ID */
	ushort gid; /* Group ID */
	ushort links_count; /* Hard links */
	uint type; /* type of this file, see above.*/
	uint size; /* How many bytes are in the file */
	uint blocks; /* Blocks allocated to file */
	uint direct[9]; /* Direct pointers to data blocks */
	uint indirect; /* A pointer to a sector of pointers */
	/* A pointer to a sector of pointers to sectors of pointers. */
	uint double_indirect;
} vsfs_inode;

/**
 * Find an inode in a file system. If the inode is found, load it into the dst
 * buffer and return 0. If the inode is not found, return 1.
 */
int vsfs_lookup(char* path, vsfs_inode* dst);

/**
 * Remove the file from the directory in which it lives and decrement the link
 * count of the file. If the file now has 0 links to it, free the file and
 * all of the blocks it is holding onto.
 */
int vsfs_unlink(char* path);



/**
 * Read sz bytes from inode node at the position start (start is the seek in
 * the file). Copy the bytes into dst. Return the amount of bytes read. If
 * the user has requested a read that is outside of the bounds of the file,
 * don't read any bytes and return 0.
 */
int vsfs_read(vsfs_inode* node, uint start, uint sz, void* dst);

/**
 * Write sz bytes to the inode node starting at position start. No more than
 * sz bytes can be copied from src. If the file is not big enough to hold
 * all of the information, allocate more blocks to the file.
 * WARNING: A user is allowed to seek to a position in a file that is not
 * allocated and write to that position. There can never be 'holes' in files
 * where there are some blocks allocated in the beginning and some at the end.
 * WARNING: Blocks allocated to files should be zerod if they aren't going to
 * be written to fully.
 */
int vsfs_write(vsfs_inode* node, uint start, uint sz, void* src);


#endif 
