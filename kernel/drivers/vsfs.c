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
    else if(root->blocks < 137){

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
    memmove(dst, ret_inode, sizeof(vsfs_inode));
    return inode_num;
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
int vsfs_unlink(char* path)
{
  char parent_name[1024];
  parent_path(path, parent_name);
  vsfs_inode parent;
  int parent_num = vsfs_lookup(parent_name, &parent);

  vsfs_inode child;
  int child_num = vsfs_lookup(path, &child);
  
  directent last_entry;

  directent last_entries[4];

  read_block
}

int find_free_inode(){
    
  int return_num;
  int i;
  uchar bitmap[512];
  for(i = 0; i < super->imap; i++){
    ata_readsect(start + 1 + i, bitmap);
    int j;
    for(j = 0; j < 512; j++){
      if(bitmap[j] != 0xFF){
        int k;
        for(k = 0; k < 8; k++){
          if(((1 << k) & bitmap[j]) == 0){
            bitmap[j] |= (1 << k);
            ata_writesect(start + 1 + i, bitmap); 
            return i*512 + j*8 + k;
          }
        }
      }
    }
  }
  return 0;
}

int find_free_block(){

  int return_num;
  int i;
  uchar bitmap[512];
  for(i = 0; i < super->dmap; i++){
    ata_readsect(start + 1 + i + super->imap, bitmap);
    int j;
    for(j = 0; j < 512; j++){
      if(bitmap[j] != 0xFF){
        int k;
        for(k = 0; k < 8; k++){
          if(((1 << k) & bitmap[j]) == 0){
            bitmap[j] |= (1 << k);
            uchar zero_sect[512];
            memset(zero_sect, 0, 512);
            ata_writesect(i*512 + j*8 + k, zero_sect);
            ata_writesect(start + 1 + i + super->imap, bitmap);
            return i*512 + j*8 + k;
          }
        }
      }
    }
  }
  return 0;

}

int trailing_slash(char* buff){

  char* curr = buff;
  
  while(*(curr) != NULL){
    curr++;
  }
  curr--;
  while(*(curr) == '/'){
    *(curr) = NULL;
    curr--;
  }
  return 0;

}

int add_block_to_inode(vsfs_inode* inode, uint blocknum)
{
  int block_index = inode->size / 512;
  
  if(block_index < 9){
   inode->direct[block_index] = blocknum;
  }
  else if(block_index < 137){
    int ind_index = block_index - 9;
    if(ind_index == 0){
      inode->indirect = find_free_block();
    }
    uint indirect_block[128];
    ata_readsect(inode->indirect, indirect_block);
    indirect_block[ind_index] = blocknum;
    ata_writesect(inode->indirect, indirect_block);
  }
  else if(block_index < 16521){
    int ind_ind_index = block_index - 137; 
    int index1 = ind_ind_index / 128;
    int index2 = ind_ind_index % 128;
    if(ind_ind_index == 0){
      inode->double_indirect = find_free_block();
    } 
    if(ind_ind_index % 128 == 0){
      uint double_block[128];
      ata_readsect(inode->double_indirect, double_block);
      double_block[index1] = find_free_block;
      ata_writesect(inode->double_indirect, double_block);
    }
      uint dindirect[128];
      uint indirect[128];
      ata_readsect(inode->double_indirect, dindirect);
      ata_readsect(dindirect[index1], indirect);
      indirect[index2] = blocknum;
      ata_writesect(dindirect[index1], indirect);
  }
  else{
    //gg
    return -1;
  }
  return 0;
}

int read_block(vsfs_inode* inode, uint block_index, void* dst)
{
  if(block_index < 9){
   ata_readsect(inode->direct[block_index], dst);
  }
  else if(block_index < 137){
    int ind_index = block_index - 9;
    uint indirect_block[128];
    ata_readsect(inode->indirect, indirect_block);
    ata_readsect(indirect_block[ind_index], dst);
  }
  else if(block_index < 16521){
    int ind_ind_index = block_index - 137; 
    int index1 = ind_ind_index / 128;
    int index2 = ind_ind_index % 128;
    uint dindirect[128];
    uint indirect[128];
    ata_readsect(inode->double_indirect, dindirect);
    ata_readsect(dindirect[index1], indirect);
    ata_readsect(indirect[index2], dst);
  }
  else{
    //gg
    return -1;
  }
  return 0; 
}

int write_block(vsfs_inode* inode, uint block_index, void* src)
{
  if(block_index < 9){
   ata_writesect(inode->direct[block_index], src);
  }
  else if(block_index < 137){
    int ind_index = block_index - 9;
    uint indirect_block[128];
    ata_readsect(inode->indirect, indirect_block);
    ata_writesect(indirect_block[ind_index], src);
  }
  else if(block_index < 16521){
    int ind_ind_index = block_index - 137; 
    int index1 = ind_ind_index / 128;
    int index2 = ind_ind_index % 128;
    uint dindirect[128];
    uint indirect[128];
    ata_readsect(inode->double_indirect, dindirect);
    ata_readsect(dindirect[index1], indirect);
    ata_writesect(indirect[index2], src);
  }
  else{
    //gg
    return -1;
  }
  return 0; 
}


int allocate_directent(vsfs_inode* parent, char* name, unint inode_num){

  directent new_directory;

  memmove(&new_directory.name, name, 124);
  new_directory.inode_num = inode_num;

  if(parent->size % 512 == 0){
    int block_num = find_free_block();
    add_block_to_inode(parent, block_num);  
  }
  directent entries[4];
  read_block(parent, parent->size / 512 , entries); 

  int i;
  for(i = 0; i < (512 / sizeof(directent)); i++){
    if(entries[i]->inode_num == 0){
      break;
    }
  }
  memmove(&entries[i], &new_directory, sizeof(directent));
  write_block(parent, parent->size / 512, entries);

  parent->size += sizeof(directent);

}

void write_inode(uint inode_num, vsfs_inode* inode)
{
  vsfs_inode inodes[512/sizeof(vsfs_inode)];
  ata_readsect(start + 1 + super->imap + super->dmap + inode_num/(512/sizeof(vsfs_inode)), inodes);
  uint offset = inode_num % (512/sizeof(vsfs_inode));
  memmove(&inodes[offset], inode, sizeof(vsfs_inode));
  ata_writesect(start + 1 + super->imap + super->dmap + inode_num/(512/sizeof(vsfs_inode)), inodes);
}

void read_inode(uint inode_num, vsfs_inode* inode)
{
  vsfs_inode inodes[512/sizeof(vsfs_inode)];
  ata_readsect(start + 1 + super->imap + super->dmap + inode_num/(512/sizeof(vsfs_inode)), inodes);
  uint offset = inode_num % (512/sizeof(vsfs_inode));
  memmove(inode, &inodes[offset], sizeof(vsfs_inode));
}

void file_name(char *path, char* dst)
{
  while(*(path) != NULL){
    path++;
  }
  while(*(path) != '/'){
    path--;
  }
  path++;

  memmove(dst, path, strlen(path));

}

void parent_path(char *path, char* dst)
{
  memmove(dst, path, strlen(path));

  while(*(dst) != NULL){
    dst++;
  }
  while(*(dst) != '/'){
    dst--;
  }
  *(dst) = NULL;
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
  char* curr = path;
  while(*(curr) != NULL){
    curr++;
  }
  while(*(curr) != '/'){
    curr--;
  }
  *(curr) = NULL;

  int parent_num = vsfs_lookup(path, parent);
  if(parent_num != 0){
    // where to put directent
    // traverse to get next free inode
    int inode_num = find_free_inode();
    if(inode_num == 0){
      return 1;
    }
    else{
      curr++;
      allocate_directent(parent, curr, inode_num);
      new_inode->links_count = 1;
      new_inode->size = 0;
      new_inode->blocks = 0;
      write_inode(parent_num, parent);
      write_inode(inode_num, new_inode);
      return 0;
    }
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
  vsfs_inode link_inode;
  int link_num = vsfs_lookup(link, &link_inode);
  if(link_num == 0) { return 1;}
  char parent[1024];
  parent_path(new_file, parent);

  vsfs_inode parent_inode;

  int parent_num = vsfs_lookup(parent, &parent_inode);
  if(parent_num == 0) { return 1;}
  char child[124];
  file_name(new_file, child);
  
  allocate_directent(&parent_inode, child, link_num);

  write_inode(parent_num, &parent_inode);
  link_inode.links_count++;
  write_inode(link_num, &link_inode);
  return 0;
}

/**
 * Create a soft link called new_file that points to link.
 */
int vsfs_soft_link(char* new_file, char* link)
{
  // ?
  return 69;
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
