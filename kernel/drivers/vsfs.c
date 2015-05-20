// VSFS driver
#include "vsfs.h"
#include "ata.h"

vsfs_superblock* super;
int start;

#define BLOCKSIZE 512
/**
 * Setup the file system driver with the file system starting at the given
 * sector. The first sector of the disk contains the super block (see above).
 */
int init(int start_sector)
{
  start = start_sector;
  uchar super_block[512];
  ata_readsect(start_sector, super_block);
  super = (vsfs_superblock*) super_block;
  return 0;
}

/**
 * Find an inode in a file system. If the inode is found, load it into the dst
 * buffer and return 0. If the inode is not found, return 1.
 */
int vsfs_lookup(char* path, vsfs_inode* dst)
{  
  char paths[16][64];
  directent* nextlevel = NULL;
  if(path[0] == '/'){
    int depth = vsfs_path_split(path, paths);
    inode_num = 1;
    int k;
    for(k = 0; k < (depth - 2); k++){
    if((k == 0) || (paths[k][0] != '/')){
    uchar traversal_inode[512];
    ata_readsect(start + 1 + super->dmap + super->imap + (inode_num / 512) , traversal_inode);
    vsfs_inode* curr = ((vsfs_inode*) traversal_inode) + (inode_num % 8);
    int i;
    int directents = curr->size / sizeof(directent);
    if(root->blocks <= 9){
      for(i = 0; i < curr->blocks; i++){
        uchar datablock[512];
        ata_readsect(curr->direct[i], datablock);
        directent *subentry = (directent*) datablock;
        int j;
        int r = 1;
        for(j = 0; (j < 4) && (directents > 0); j++){
          if(strcmp(subentry->name, paths[k+r]) == 0){
            if(r == 1){ r = 2;};
            nextlevel = subentry;
            inode_num = firstlevel->inode_num;
          } 
          else{
            subentry++;
            directents--;
          }
        }
        if(directents == 0){
          return 0;
        }
      }
    }
    else if(root->blocks <= 81){

    }
    else{

    }
    }     
  } 
  if(nextlevel == NULL){
    return 0;
  }   
  else{
    uchar final_inode[512];
    ata_readsect(start + 1 + super->dmap + super->imap + (inode_num / 512), final_inode);
    vsfs_inode* ret_inode = ((vsfs_inode*) final_inode) + (inode_num % 8);
    dst = ret_inode;
    return 1;
  }

  else{
    return 0;
  }  
}

/**
 * Splits the file/folder path into a 2D array and returns it.
 */
int vsfs_path_split(char* path, char** dst){

  int i = 0;
  int j = 0;
  int k = 0;
  while(path[i] != NULL){
    if(path[i] == '/'){
      dst[j][0] = '/';
      j++;
      k = 0;
    }
    else{
      dst[j][k] = path[i];
    }
    i++;
  }
  return j;
}

/**
 * Remove the file from the directory in which it lives and decrement the link
 * count of the file. If the file now has 0 links to it, free the file and
 * all of the blocks it is holding onto.
 */
int vsfs_unlink(char* path){

  vsfs_inode* unlinked;
  
  if(vsfs_lookup(path, unlinked) == 1){
    unlinked->links_count--;
    if(unlinked->links_count == 0){
      //free in the bitmap
      //free data blocks in bitmap
    }

  }
  else{
    return 0;
  }
}

/**
 * Add the inode new_inode to the file system at path. Make sure to add the
 * directory entry in the parent directory. If there are no more inodes
 * available in the file system, or there is any other error return 1.
 * Return 0 on success.
 */
int vsfs_link(char* path, vsfs_inode* new_inode)
{
  vsfs_inode* parent;

  if(vsfs_lookup(path, parent) == 1){
    // where to put directent
    // traverse to get next free inode
    bit search =
  
  }
  else{
    return 1;
  }
}

/**
 * Create the directory entry new_file that is a hard link to file. Return 0
 * on success, return 1 otherwise.
 */
int vsfs_hard_link(char* new_file, char* link)
{
  // ?
}

/**
 * Create a soft link called new_file that points to link.
 */
int vsfs_soft_link(char* new_file, char* link)
{
  // ?
}

/**
 * Read sz bytes from inode node at the position start (start is the seek in
 * the file). Copy the bytes into dst. Return the amount of bytes read. If
 * the user has requested a read that is outside of the bounds of the file,
 * don't read any bytes and return 0.
 */
int vsfs_read(vsfs_inode* node, uint start, uint sz, void* dst)
{
  // ?
}

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
int vsfs_write(vsfs_inode* node, uint start, uint sz, void* src)
{
  // ?
}
