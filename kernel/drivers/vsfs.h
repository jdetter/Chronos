// VSFS header


typedef struct fuckininode{

short mode;
short uid;
int size;
short gid;
short links_count;
int blocks;
int flags;
int block[15];
int file_acl;
int dir_acl;

}inode;
