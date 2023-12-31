#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include "inode.h"
#include "bitmap.h"

// Initializes and reserves space for the inode table.
// NOTE: Reserves 0th inode so reference the 0th inode
//       is equivalent to pointing to nothing.
void inode_init() {
  void *bbm = get_blocks_bitmap();
  // reserves space for inode table
  int blocks = bytes_to_blocks(INODE_COUNT * INODE_SIZE);
  for (int ii = 1; ii < blocks; ++ii) {
    bitmap_put(bbm, ii, 1);
  }
  // reserves the 0th inode.
  void *ibm = get_inode_bitmap();
  bitmap_put(ibm, 0, 1);
}

// Gets the inode at the given index in the inode table.
// NOTE: inodes are zero indexed.
inode_t *get_inode(int inum) {
  // checks if the inode index is out of bounds.
  // the 0th inode is considered out of bounds.
  if (inum <= 0 || INODE_COUNT <= inum) {
    return NULL;
  }
  char *table = blocks_get_block(1);
  return (inode_t *) (table + INODE_SIZE * inum);
}

// Allocate a new inode and return its index.
int alloc_inode(int mode) {
  void *ibm = get_inode_bitmap();
  // allocates next unused inode in inode table.
  // skips inode 0 and 1 since they are reserved.
  for (int ii = 2; ii < INODE_COUNT; ++ii) {
    if (!bitmap_get(ibm, ii)) {
      inode_t *node = get_inode(ii);
      bitmap_put(ibm, ii, 1);
      node->mode = mode;
      node->inum = ii;
      printf("+ alloc_inode() -> %d\n", ii);
      return ii;
    }
  }
  return -1;
}

// Deallocate the inode with the given number.
void free_inode(int inum) {
  void *ibm = get_inode_bitmap();
  // checks if inode was already freed.
  if (bitmap_get(ibm, inum)) {
    inode_t *node = get_inode(inum);
    shrink_inode(node, 0);
    bitmap_put(ibm, inum, 0);
  }
}

// Checks if the given inode is valid and inuse.
int inode_valid(inode_t *node) {
  void *ibm = get_inode_bitmap();
  if (NULL == node || !bitmap_get(ibm, node->inum) || 0 == node->inum) {
    return 0;
  }
  return 1;
}

// Get a pointer to the block number of the file block of
// the given index.
int *inode_get_bnum(inode_t *node, int file_bnum) {
  // checks if file block of the given index can exist.
  if (!inode_valid(node) || file_bnum < 0 || NINDIRECT + NDIRECT <= file_bnum) {
   return NULL;
  }
  // the first NDIRECT bnums are stored in the direct array.
  if (file_bnum < NDIRECT) {
   return &node->direct[file_bnum];
  }
  // checks to see if the indirect block has been allocated.
  if (0 == node->indirect) {
   return NULL;
  }
  // the rest of the bnums are stored in the indirect block.
  int *indirect = blocks_get_block(node->indirect);
  return &indirect[file_bnum - NDIRECT];
}

// Gets a pointer to the file byte of the given index.
char *inode_get_byte(inode_t *node, int file_byte) {
  // checks if the byte index is in bounds of the file.
  if (!inode_valid(node) || file_byte < 0 || node->size <= file_byte) {
    return NULL;
  }
  // block where byte is has definitely been allocated.
  int bnum = file_byte / BLOCK_SIZE;
  bnum = *inode_get_bnum(node, bnum);
  char *ptr = blocks_get_block(bnum);
  return ptr + file_byte % BLOCK_SIZE;
}

// Attempts to allocate a indirect block to the given inode.
// Returns 0 if successful, or -1 if unsuccessful.
int alloc_indirect(inode_t *node) {
  int bnum = alloc_block();
  if (-1 == bnum) {
    return -1;
  }
  node->indirect = bnum;
  return 0;
}

// Increases size of inode. Returns -1 if operation fails.
int grow_inode(inode_t *node, int size) {
  // checks if arguments are valid.
  if (!inode_valid(node) || size < node->size) {
    return -1;
  }

  int curr_bcount = bytes_to_blocks(node->size);
  int target_bcount = bytes_to_blocks(size);

  while (curr_bcount < target_bcount) {
    // NOTE: takes advantage of short-circuit logical operators
    // to work. if the direct block pointers are all already
    // in use and the indirect block has yet to be allocated,
    // and the allocation of the indirect block fails (which
    // only runs if the other two conditions are fulfilled),
    // then abort.
    if (NDIRECT <= curr_bcount && 0 == node->indirect && -1 == alloc_indirect(node)) {
      node->size = BLOCK_SIZE * curr_bcount;
      return -1;
    }
    int bnum = alloc_block();
    // aborts if block allocation fails.
    if (-1 == bnum) {
      node->size = BLOCK_SIZE * curr_bcount;
      return -1;
    }
    // assigns the next file block.
    *inode_get_bnum(node, curr_bcount) = bnum;
    ++curr_bcount;
  }

  node->size = size;
  return 0;
}

// Decrease size of inode, Returns -1 if operation fails.
// NOTE: Assumes there are no gaps in the file block cache.
int shrink_inode(inode_t *node, int size) {
  // checks if arguments are valid.
  if (!inode_valid(node) || node->size < size) {
    return -1;
  }

  int curr_bcount = bytes_to_blocks(node->size);
  int target_bcount = bytes_to_blocks(size);

  while (target_bcount < curr_bcount) {
    int *block = inode_get_bnum(node, curr_bcount - 1);
    // aborts if file block is out of bounds or there is a gap
    // in the file block cache.
    if (NULL == block || 0 == *block) {
      continue;
    }

    free_block(*block);
    *block = 0;
    --curr_bcount;
  }

  // deallocates indirect block if the indirect block is empty.
  if (target_bcount <= NDIRECT && 0 != node->indirect) {
    free_block(node->indirect);
    node->indirect = 0;
  }

  node->size = size;
  return size;
}

// Reads the given number of bytes from the given file into
// the given buffer starting at the given byte index.
int inode_read(inode_t *node, char *buf, int offset, int n) {
  if (!inode_valid(node) || offset < 0 || n < 0) {
    return -1;
  }

  int i = 0;
  while(i < n) {
    if (node->size <= offset + i) {
      return i;
    }
    // byte is definitely accessible
    *buf = *inode_get_byte(node, offset + i);
    ++buf;
    ++i;
  }
  return i;
}

// Writes the given bytes to the given file starting at
// the given byte index.
int inode_write(inode_t *node, const char *buf, int offset, int n) {
  if (!inode_valid(node) || offset < 0 || n <= 0) {
    return -1;
  }

  // ensures enough blocks before writing.
  grow_inode(node, offset + n);

  int i = 0;
  while(i < n) {
    if (node->size <= offset + i) {
      // FUSE documentation says write cannot return 0.
      return (i == 0) ? -1 : i;
    }
    // byte is definitely accessible
    *inode_get_byte(node, offset + i)= *buf;
    ++buf;
    ++i;
  }

  return i;
}

// Copies the stat information of given file into the given
// stat struct.
int inode_stat(inode_t *node, struct stat *st) {
  if (!inode_valid(node) || NULL == st) {
    return -1;
  }
  st->st_ino = node->inum;
  st->st_mode = node->mode;
  st->st_nlink = node->links;
  st->st_uid = getuid();
  st->st_gid = getgid();
  st->st_blksize = BLOCK_SIZE;
  st->st_size = node->size;
  st->st_blocks = bytes_to_blocks(node->size);
  // Time is not supported.
  st->st_atime = 0;
  st->st_mtime = 0;
  st->st_ctime = 0;
  return 0;
}

// prints the block numbers stored in the given
// block number cache.
// NOTE: Assumes there are no gaps in the cache.
void print_inode_bnums(int *cache, int len) {
  for (int ii = 0; ii < len; ++ii) {
    if (0 == cache[ii]) {
      return;
    }
    printf("  %d\n", cache[ii]);
  }
}

// Prints the state of the given inode in a readable format.
void print_inode(inode_t *node) {
  if (!inode_valid(node)) {
    printf("N/A\n");
    return;
  }
  // prints inode info
  printf("inum: %d\nmode: %o\nlinks: %d\nrefs: %d\nsize: %d\n",
         node->inum, node->mode, node->links, node->refs, node->size);
  // prints block indexes of blocks inode owns
  printf("blocks:\n");
  print_inode_bnums(node->direct, NDIRECT);
  // returns if indirect block has not been allocated
  if (0 == node->indirect) {
    return;
  }
  // prints block indexes referred to in the indirect block
  int *indirect = blocks_get_block(node->indirect);
  print_inode_bnums(indirect, NINDIRECT);
}

// Reads the mode flags using bitmasks.
// NOTE: Assume the caller is the owner of the file.
void read_mode(int mode, int *isdirectory, int *isfile,
               int *canread, int *canwrite, int *canexecute) {
  if (isdirectory != NULL && ((mode & S_IFDIR) > 0))
    *isdirectory = 1;
  if (isfile != NULL && ((mode & S_IFREG) > 0))
    *isfile = 1;
  if (canread != NULL && ((mode & S_IRUSR) > 0))
    *canread = 1;
  if (canwrite != NULL && ((mode & S_IWUSR) > 0))
    *canwrite = 1;
  if (canexecute != NULL && ((mode & S_IXUSR) > 0))
    *canexecute = 1;
}
