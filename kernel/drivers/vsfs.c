// VSFS driver

#ifndef VSFS_MKFS 
#include "types.h"
#include "stdarg.h"
#include "stdlib.h"
#else

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

#include <stdlib.h>
#include <string.h>

#endif

#include "file.h"
#include "fsman.h"
#include "vsfs.h"

/* Forward declarations */
int vsfs_path_split(char* path, char* dst[MAX_PATH_DEPTH]);
void free_inode(uint inode_num, struct vsfs_context* context);
void free_block(uint block_num, struct vsfs_context* context);
int find_free_inode(struct vsfs_context* context);
int find_free_block(struct vsfs_context* context);
int trailing_slash(char* buff);
int add_block_to_inode(vsfs_inode* inode, uint blocknum,
		struct vsfs_context* context);
int read_block(vsfs_inode* inode, uint block_index, void* dst,
		struct vsfs_context* context);
int write_block(vsfs_inode* inode, uint block_index, void* src,
		struct vsfs_context* context);
int allocate_directent(vsfs_inode* parent, char* name, uint inode_num,
		struct vsfs_context* context);
void write_inode(uint inode_num, vsfs_inode* inode,
		struct vsfs_context* context);
void read_inode(uint inode_num, vsfs_inode* inode, 
		struct vsfs_context* context);
void file_name(const char *path_orig, char* dst);
void parent_path(const char *path, char* dst);


#define BLOCKSIZE 512
/** Start standardized vsfs library */

/**
 * Setup the file system driver with the file system starting at the given
 * sector. The first sector of the disk contains the super block (see above).
 */
int vsfs_init(uint start_sector, uint end_sector, uint block_size,
		struct vsfs_context* context, uchar* cache, uint cache_sz)
{
  context->start = start_sector;
  uchar super_block[512];
  context->hdd->read(super_block, start_sector, context->hdd);
  memmove(&context->super, super_block, sizeof(vsfs_superblock));
  context->imap_off = context->start + 1;
  context->bmap_off = context->imap_off + context->super.imap;
  context->i_off = context->bmap_off + context->super.dmap;
  context->b_off = context->i_off + 
    (context->super.inodes / (512 / sizeof(vsfs_inode)));

  context->read = context->hdd->read;
  context->write = context->hdd->write;

 return 0;
}

void* vsfs_open(const char* path, uint flags, uint permissions, 
		struct vsfs_context* context)
{
		
}

/** End standardized vsfs library */

struct vsfs_inode* vsfs_alloc_inode(void)
{
	
}

int vsfs_lookup(const char* path_orig, vsfs_inode* dst, 
	struct vsfs_context* context)
{
	char path[strlen(path_orig) + 1];
	strncpy(path, path_orig, strlen(path_orig) + 1);
	char* path_split[MAX_PATH_DEPTH];
	memset(path_split, 0, MAX_PATH_DEPTH);
	int max_depth = vsfs_path_split(path, path_split);	
	if(max_depth == 0) return 0;
	
	int inode_num = 1;
	struct vsfs_inode curr_inode;
	read_inode(1, &curr_inode, context);
	int path_pos = 1;

	int entries_per_block = 512 / sizeof(struct directent);
	char sector[512];
	struct directent* entries = (struct directent*)sector;
	while(path_split[path_pos] && path_pos < max_depth)
	{
		/* Enumerate the current inode. */
		int found = 0;
		int x;
		for(x = 0;x < curr_inode.blocks;x++)
		{
			read_block(&curr_inode, x, sector, context);
			int entry;
			for(entry = 0;entry < entries_per_block; entry++)
			{
				if(!strcmp(entries[entry].name, 
					path_split[path_pos]))
				{
					found = 1;
					
					path_pos++;
					/* Get the next inode. */
					inode_num = entries[entry].inode_num;
					read_inode(inode_num, 
						&curr_inode, context);

					/* If were at the end, the break.*/
					if(path_pos == max_depth) break;

					/* 
					 * If this isn't a directory, somthing
					 * is wrong.
					 */
					if(curr_inode.type != VSFS_DIR)	
						return 0;
					break;
				}
			}

			if(found) break;
		}

		if(!found)
		{
			/* No such directory error */
			return 0;
		}
	}

	/* We should be at the final path element now. */
	memmove(dst, &curr_inode, sizeof(struct vsfs_inode));
	/* return the inode number we found */
	return inode_num;
}

/**
 * Splits the path into a 2D array and returns the
 * count of directories.
 */
int vsfs_path_split(char* path, char* dst[MAX_PATH_DEPTH])
{
	/* Absolute paths only */
	if(path[0] != '/') return 0;
	dst[0] = 0; 
	if(!strcmp(path, "/"))
	{
		dst[1] = 0;
		return 1;
	}
	dst[1] = path + 1;
	char* c = path + 1;
	int x;
	for(x = 2;x < MAX_PATH_DEPTH && *c;)
	{
		if(*c == '/')
		{
			/* Set *c to zero and increment. */
			*c = 0;c++;
			/* Is this the end of the string? */
			if(!*c) break;
			/* There is a new section */
			dst[x] = c;
			x++;
		} else c++;
	}

	dst[x] = 0;
	return x;
}

void free_inode(uint inode_num, struct vsfs_context* context)
{
  vsfs_inode tofree;
  read_inode(inode_num, &tofree, context);
  int i;
  for(i = 0; i < (tofree.size + 511)/512; i++){
    if(i < 9){
   free_block(tofree.direct[i], context);
  }
  else if(i < 137){
    int ind_index = i - 9;
    uint indirect_block[128];
    context->hdd->read(indirect_block, tofree.indirect, context->hdd);
    free_block(indirect_block[ind_index], context);
  }
  else if(i < 16521){
    int ind_ind_index = i - 137; 
    int index1 = ind_ind_index / 128;
    int index2 = ind_ind_index % 128;
    uint dindirect[128];
    uint indirect[128];
    context->read(dindirect, tofree.double_indirect, context->hdd);
    context->read(indirect, dindirect[index1], context->hdd);
    free_block(indirect[index2], context);
  }
  } 
  uchar free[512];
  context->read(free, context->start + 1 + inode_num/4096, context->hdd);
  uint inode_index = inode_num % 4096;
  uint inode_byte =  free[inode_index / 8];
  uchar mask = (1 << (inode_index % 8));
  mask = ~mask;
  free[inode_index / 8] = inode_byte & mask;
  context->write(free, context->start + 1 + inode_num/4096, context->hdd); 
}

void free_block(uint block_num, struct vsfs_context* context)
{
  block_num -= context->start + 1 + 
	context->super.imap + context->super.dmap;
  uchar free[512];
  context->read(free, context->start + 1 
	+ context->super.imap + block_num/4096, context->hdd);
  uint block_index = block_num % 4096;
  uint block_byte =  free[block_index / 8];
  uchar mask = (1 << (block_index % 8));
  mask = ~mask;
  free[block_index / 8] = block_byte & mask;
  context->write(free, context->start + 1 + context->super.imap 
	+ block_num/4096, context->hdd);
}

/**
 * Remove the file from the directory in which it lives and decrement the link
 * count of the file. If the file now has 0 links to it, free the file and
 * all of the blocks it is holding onto.
 */
int vsfs_unlink(const char* path, struct vsfs_context* context)
{
  char parent_name[1024];
  parent_path(path, parent_name);
  vsfs_inode parent;
  int parent_num = vsfs_lookup(parent_name, &parent, context);

  vsfs_inode child;
  int child_num = vsfs_lookup(path, &child, context);
  
  directent last_entry;
  directent entries[4];
  uint last_entry_inum;

  read_block(&parent, parent.size / 512, entries, context);
  int last_index = 0;
  for(last_index = 0; last_index < 4; last_index++){
    if(entries[last_index].inode_num == 0){
      last_index--;
      last_entry_inum = entries[last_index].inode_num;
      break;
    }
  }
  memmove(&last_entry, &entries[last_index], sizeof(directent));
  
  int blocks;
  int done = 0;
  for(blocks = 0; !done && blocks < ((parent.size+ 511)/512); blocks++){
    read_block(&parent, blocks, entries, context);
    int i;
    for(i = 0; i < (512/sizeof(directent)); i++){
      if(entries[i].inode_num == child_num){
        child.links_count--;
        if(child.links_count == 0){
          free_inode(child_num, context);
        }
        else{
          write_inode(child_num, &child, context);
        }
        if(last_entry_inum == child_num){
          done = 1;
          break;
        } 
        memmove(&entries[i], &last_entry, sizeof(directent));
        write_block(&child, blocks, entries, context);
        parent.size -= sizeof(directent);
        write_inode(parent_num, &parent, context);
        done = 1;
      }
    }
  } 
  return 0;   
}


int find_free_inode(struct vsfs_context* context){
    
  int i;
  uchar bitmap[512];
  for(i = 0; i < context->super.imap; i++){
    context->read(bitmap, context->start + 1 + i, context->hdd);
    int j;
    for(j = 0; j < 512; j++){
      if(bitmap[j] != 0xFF){
        int k = 0;
	/* Never return inode 0. */
	if(j == 0 && k == 0) k = 1;

        for(; k < 8; k++){
          if(((1 << k) & bitmap[j]) == 0){
            bitmap[j] |= (1 << k);
            context->write(bitmap, context->start + 1 + i, context->hdd); 
            return i*512 + j*8 + k;
          }
        }
      }
    }
  }
  return 0;
}

int find_free_block(struct vsfs_context* context){
	uchar zero_sect[512];
	memset(zero_sect, 0, 512);

	uchar sect[512];
	int bmap;
	for(bmap = 0;bmap < context->super.dmap;bmap++)
	{
		/* read the bitmap */
		context->read(sect, context->bmap_off 
			+ bmap, context->hdd);

		/* Search for a byte, b such that: b != 0xFF */
	
		int x;	
		/* Note: make sure we skip block 0 for debugging. */
		for(x = 1;x < 512;x++)
		{
			uchar val = sect[x];
			if(val != 0xFF)
			{
				/* Found one. Now get the bit. */
				int bit;
				for(bit = 0;val & 0x01;bit++)
					val >>= 1;
				/* Calculate sector number */
				int sector_num = 
					bmap * (512 * 8)
					+ x * 8
					+ bit;
				/* allocate this block. */
				sect[x] |= 1 << bit;
				context->write(sect, context->bmap_off 
					+ bmap, context->hdd);

				/* Return the sector.*/
				return sector_num;
			}
		}
	}
	
	return 0;
}

int trailing_slash(char* buff){

  char* curr = buff;
  
  while(*(curr) != 0){
    curr++;
  }
  curr--;
  while(*(curr) == '/'){
    *(curr) = 0;
    curr--;
  }
  return 0;

}

int add_block_to_inode(vsfs_inode* inode, uint blocknum,
		struct vsfs_context* context)
{
  int block_index = inode->blocks;

  if(block_index < 9){
   inode->direct[block_index] = blocknum;
  }
  else if(block_index < 137){
    int ind_index = block_index - 9;
    if(ind_index == 0){
      inode->indirect = find_free_block(context);
    }
    uint indirect_block[128];
    context->read(indirect_block, context->b_off 
		+ inode->indirect, context->hdd);
    indirect_block[ind_index] = blocknum;
    context->write(indirect_block, context->b_off 
		+ inode->indirect, context->hdd);
  }
  else if(block_index < 16521){
    int ind_ind_index = block_index - 137; 
    int index1 = ind_ind_index / 128;
    int index2 = ind_ind_index % 128;
    if(ind_ind_index == 0){
      inode->double_indirect = find_free_block(context);
    } 
    if(ind_ind_index % 128 == 0){
      uint double_block[128];
      context->read(double_block, context->b_off 
		+ inode->double_indirect, context->hdd);
      double_block[index1] = find_free_block(context);
      context->write(double_block, context->b_off 
		+ inode->double_indirect, context->hdd);
    }
      uint dindirect[128];
      uint indirect[128];
      context->read(dindirect, context->b_off 
		+ inode->double_indirect, context->hdd);
      context->read(indirect, context->b_off 
		+ dindirect[index1], context->hdd);
      indirect[index2] = blocknum;
      context->write(indirect, context->b_off 
		+ dindirect[index1], context->hdd);
  }
  else{
    //gg
    return -1;
  }

  inode->blocks++;
  return 0;
}

int read_block(vsfs_inode* inode, uint block_index, void* dst,
		struct vsfs_context* context)
{
  if(block_index < 9){
   context->read(dst, context->b_off + inode->direct[block_index], 
		context->hdd);
  }
  else if(block_index < 137){
    int ind_index = block_index - 9;
    uint indirect_block[128];
    context->read(indirect_block, context->b_off + inode->indirect,	
		context->hdd);
    context->read(dst, context->b_off + indirect_block[ind_index],
		context->hdd);
  }
  else if(block_index < 16521){
    int ind_ind_index = block_index - 137; 
    int index1 = ind_ind_index / 128;
    int index2 = ind_ind_index % 128;
    uint dindirect[128];
    uint indirect[128];
    context->read(dindirect, context->b_off + inode->double_indirect, context->hdd);
    context->read(indirect, context->b_off + dindirect[index1], context->hdd);
    context->read(dst, context->b_off + indirect[index2], context->hdd);
  }
  else{
    //gg
    return -1;
  }
  return 0; 
}

int write_block(vsfs_inode* inode, uint block_index, void* src,
		struct vsfs_context* context)
{
  if(block_index < VSFS_DIRECT){
   context->write(src, context->b_off + 
	inode->direct[block_index], context->hdd);
  }
  else if(block_index < 137){
    int ind_index = block_index - 9;
    uint indirect_block[128];
    context->read(indirect_block, context->b_off 
		+ inode->indirect, context->hdd);
    context->write(src, context->b_off 
		+ indirect_block[ind_index], context->hdd);
  }
  else if(block_index < 16521){
    int ind_ind_index = block_index - 137; 
    int index1 = ind_ind_index / 128;
    int index2 = ind_ind_index % 128;
    uint dindirect[128];
    uint indirect[128];
    context->read(dindirect, context->b_off + inode->double_indirect, context->hdd);
    context->read(indirect, context->b_off + dindirect[index1], context->hdd);
    context->write(src, context->b_off + indirect[index2], context->hdd);
  } else {
    // file is too large
    return -1;
  }
  return 0; 
}

/**
 * Add a directory entry to a file.
 */
int allocate_directent(vsfs_inode* parent, char* name, uint inode_num,
		struct vsfs_context* context)
{
  directent new_directory;
  memset(&new_directory, 0, sizeof(directent));

  strncpy((char*)&new_directory.name, name, VSFS_MAX_NAME);
  new_directory.inode_num = inode_num;
  int block = parent->size / 512;

  if(parent->size % 512 == 0)
  {
    int block_num = find_free_block(context);
    add_block_to_inode(parent, block_num, context);  
  }

  directent entries[512 / sizeof(directent)];
  read_block(parent, block , entries, context); 

  int i;
  for(i = 0; i < (512 / sizeof(directent)); i++){
    if(entries[i].inode_num == 0){
      break;
    }
  }
  memmove(&entries[i], &new_directory, sizeof(directent));
  parent->size += sizeof(directent);
  write_block(parent, block, entries, context);

  /* Increment child's link count */
  vsfs_inode child_inode;
  read_inode(inode_num, &child_inode, context);
  child_inode.links_count++;
  write_inode(inode_num, &child_inode, context);

  return 0;
}

void write_inode(uint inode_num, vsfs_inode* inode,
		struct vsfs_context* context)
{
  vsfs_inode inodes[512/sizeof(vsfs_inode)];
  context->read(inodes, context->i_off 
	+ inode_num/(512/sizeof(vsfs_inode)), context->hdd);
  uint offset = inode_num % (512/sizeof(vsfs_inode));
  memmove(&inodes[offset], inode, sizeof(vsfs_inode));
  context->write(inodes, context->i_off 
	+ inode_num/(512/sizeof(vsfs_inode)), context->hdd);
}

void read_inode(uint inode_num, vsfs_inode* inode,
		struct vsfs_context* context)
{
  vsfs_inode inodes[512/sizeof(vsfs_inode)];
  context->read(inodes, context->i_off 
		+ inode_num/(512/sizeof(vsfs_inode)), context->hdd);
  uint offset = inode_num % (512/sizeof(vsfs_inode));
  memmove(inode, &inodes[offset], sizeof(vsfs_inode));
}

void file_name(const char *path_orig, char* dst)
{
  char path_buffer[strlen(path_orig) + 1];
  memmove(path_buffer, (char*)path_orig, strlen(path_orig) + 1);
  char* path = path_buffer;
  while(*(path) != 0){
    path++;
  }
  while(*(path) != '/'){
    path--;
  }
  path++;

  memmove(dst, path, strlen(path));
}

void parent_path(const char *path, char* dst)
{
  memmove(dst, (char*)path, strlen(path));

  while(*(dst) != 0){
    dst++;
  }
  while(*(dst) != '/'){
    dst--;
  }
  *(dst) = 0;
}

int vsfs_link(const char* path, vsfs_inode* new_inode,
		struct vsfs_context* context)
{
  /* Create a temporary buffer for our path*/
  char tmp_path[1024];
  strncpy(tmp_path, path, 1024);

  /* Find the parent directory.*/
  vsfs_inode parent;
  char* curr = tmp_path + strlen(path);
  while(*(curr) != '/'){
    curr--;
  }
  *(curr) = 0;

  /* Check to see if parent is the root */
  int parent_num = 0;
  if(strlen(tmp_path) == 0) 
  {
     parent_num = 1;
     read_inode(1, &parent, context);
  }  else parent_num = vsfs_lookup(tmp_path, &parent, context);

  if(parent_num != 0)
  {
    int inode_num = find_free_inode(context);
    if(inode_num == 0)
    {
      return -1;
    } else {
      curr++;
      allocate_directent(&parent, curr, inode_num, context);
      new_inode->links_count = 1;
      new_inode->size = 0;
      new_inode->blocks = 0;

      if(new_inode->type == VSFS_DIR)
      {
          write_inode(parent_num, &parent, context);
          write_inode(inode_num, new_inode, context);
          /* If this is a directory, do . and ..*/
          allocate_directent(new_inode, ".", inode_num, context);
          allocate_directent(new_inode, "..", parent_num, context);
      }
      write_inode(parent_num, &parent, context);
      write_inode(inode_num, new_inode, context);
      return inode_num;
    }
  }

  return -1;
}

/**
 * Create the directory entry new_file that is a hard link to file. Return 0
 * on success, return 1 otherwise.
 */
int vsfs_hard_link(const char* new_file, const char* link,
		struct vsfs_context* context)
{
  vsfs_inode link_inode;
  int link_num = vsfs_lookup(link, &link_inode, context);
  if(link_num == 0) { return 1;}
  char parent[1024];
  parent_path(new_file, parent);

  vsfs_inode parent_inode;

  int parent_num = vsfs_lookup(parent, &parent_inode, context);
  if(parent_num == 0) { return 1;}
  char child[124];
  file_name(new_file, child);
  
  allocate_directent(&parent_inode, child, link_num, context);

  write_inode(parent_num, &parent_inode, context);
  link_inode.links_count++;
  write_inode(link_num, &link_inode, context);
  return 0;
}

/**
 * Create a soft link called new_file that points to link.
 */
int vsfs_soft_link(const char* new_file, const char* link,
		struct vsfs_context* context)
{
  // ?
  return 69;
}

/**
 * Read sz bytes from inode node at the position start (start is the seek in
 * the file). Copy the bytes into dst. Return the amount of bytes read. If
 * the user has requested a read that is outside of the bounds of the file,
 * don't read any bytes and return -1.
 */
int vsfs_read(vsfs_inode* node, uint start, uint sz, void* dst,
		struct vsfs_context* context)
{
  if((start + sz) > node->size){
    return -1;
  }

  char* dst_c = (char*) dst;
  uint startblock = start / 512;
  uint endblock = (start + sz) / 512;
  uint curr = 0;
  uint currblock = 0;
  uchar block[512];
  for(curr = 0; curr < sz; currblock++){
    read_block(node, currblock + startblock, block, context);
    if(startblock == endblock)
    {
      uint start_read = start % 512;
      memmove(dst_c, block + start_read, sz);
      curr = sz;
    } else if(curr == 0){
      uint start_read = start % 512;
      memmove(dst_c, block + start_read, 512 - start_read);
      curr += (512 - start_read);
    }
    else if(currblock == endblock){
      memmove(dst_c + curr, block, (start + sz) % 512);
      curr = sz;
      break;
    }
    else{
      memmove(dst_c + curr, block, 512);
      curr += 512;
    }
  }
  return sz;
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
int vsfs_write(vsfs_inode* node, uint start, uint sz, void* src,
		struct vsfs_context* context)
{
  if((start + sz) > node->size){
    int newblocks = ((start + sz + 511)/512) - node->blocks;
    int i;        
    for(i = 0; i < newblocks; i++){
      int new_num = find_free_block(context);
      add_block_to_inode(node, new_num, context);
    }

    /* Update metadata */
    node->size = start + sz;
  }

  uint startblock = start / 512;
  uint endblock = (start + sz) / 512;
  uint curr = 0;
  uint currblock = 0;
  uchar block[512];

  for(curr = 0; curr < sz; currblock++){
    read_block(node, currblock + startblock, block, context);
    if(startblock == endblock)
    {
      uint start_write = start % 512;
      memmove(block + start_write, src, sz);
      curr = sz;
    } else if(curr == 0){
      uint start_write = start % 512;
      memmove(block + start_write, src, 512 - start_write);
      curr += (512 - start_write);
    }
    else if(currblock == endblock){
      memmove(block, src + curr, sz - curr);
      curr = sz;
    } else{
      memmove(block, src + curr, 512);
      curr += 512;
    }

    write_block(node, currblock + startblock, block, context);
  }

  return sz;
}

void vsfs_clear(vsfs_inode* node)
{
	memset(node, 0, sizeof(vsfs_inode));
}
