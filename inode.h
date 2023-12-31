// Inode manipulation routines.
#ifndef INODE_H
#define INODE_H

#include <sys/stat.h>

#include "blocks.h"

#define NDIRECT 12         // number of direct block pointers
#define NINDIRECT 1024     // number of indirect block pointers
                           // INDIRECT = BLOCK_SIZE / sizeof(int)
extern const int INODE_COUNT;  // there are at most as many inodes as blocks (default = 256)
extern const int INODE_SIZE;   // the size the inode struct in bytes (default = sizeof(inode_t))

extern const int INODE_BITMAP_SIZE; // default = 256 / 8 = 32

typedef struct inode_t {
  int inum;            // inode index
  int mode;            // permission & type
  int refs;            // reference count
  int links;           // hard-link count
  int size;            // bytes
  int direct[NDIRECT]; // direct pointers
  int indirect;        // indirect pointer
} inode_t;             // struct size : 72 bytes

/**
 * Initializes and reserves space for the inode table.
 */
void inode_init(void);

/**
 * Get the inode with the given index.
 *
 * @param inum Inode number (index).
 *
 * @return Pointer to inode.
 */
inode_t *get_inode(int inum);

/**
 * Allocate a new inode and return its number.
 *
 * @param mode Mode to set the allocated inode to.
 *
 * @return The index of the newly allocated inode, or -1
 *         if a inode cannot be allocated.
 */
int alloc_inode(int mode);

/**
 * Deallocate the inode with the given number.
 *
 * @param inum The inode number to deallocate.
 */
void free_inode(int inum);

/**
 * Checks the given inode is valid and in use.
 *
 * @param node Inode.
 *
 * @return 1 if valid, 0 if invalid.
 */
int inode_valid(inode_t *node);

/**
 * Get a pointer to the block number of the file block
 * of the given index.
 *
 * @param node Inode of the file.
 * @param file_bnum File block number (index).
 *
 * @return Pointer to the block number of the file block
 *         of the given index.
 */
int *inode_get_bnum(inode_t *node, int file_bnum);

/**
 * Gets a pointer to the file byte of the given index.
 *
 * @param node Inode of the file.
 * @param file_byte File byte number (index).
 *
 * @return Pointer to the file byte of the given index.
 */
char *inode_get_byte(inode_t *node, int file_byte);

/**
 * Grows the given file to the given size, allocating
 * blocks as needed.
 *
 * @param node Inode of the file.
 * @param size Size to grow the file to in bytes.
 *
 * @return Number of bytes the given file grew.
 */
int grow_inode(inode_t *node, int size);

/**
 * Shrinks the given file to the given size, deallocating
 * blocks as needed.
 *
 * @param node Inode of the file.
 * @param size Size to shrink the file to in bytes.
 *
 * @return Number of bytes the given file shrunk.
 */
int shrink_inode(inode_t *node, int size);

/**
 * Reads the given number of bytes from the given file into
 * the given buffer starting at the given byte index.
 *
 * @param node Inode of the file.
 * @param buf Array to read into.
 * @param offset Byte to start reading from (index).
 * @param n Number of bytes to read.
 *
 * @return The number of bytes read into the given buffer.
 */
int inode_read(inode_t *node, char*buf, int offset, int n);

/**
 * Writes the given bytes to the given file starting
 * at the given byte index.
 *
 * @param node Inode of the file.
 * @param buf Array of bytes to write.
 * @param offset Byte to start writing from (index).
 * @param n Number of bytes to write.
 *
 * @return The number of bytes written to the given file.
 */
int inode_write(inode_t *node, const char *buf, int offset, int n);

/**
 * Copies the stat information of given file into the
 * given stat struct.
 *
 * @param node Inode of the file.
 * @param st Pointer to the stat struct.
 *
 * @return Exit status of function. Returns 0 on
 * success and -1 on failure.
 */
int inode_stat(inode_t *node, struct stat *st);

/**
 * Prints the state of the given inode in the following format:
 *
 *     inum: *inode number*
 *     mode: *mode*
 *     links: *number of hard links*
 *     refs: *number of refrences*
 *     size: *file size in bytes*
 *     blocks:
 *       *file block 1*
 *       *file block 2*
 *       ...
 *
 * or prints 'N/A' if the given inode is not valid or unallocated.
 *
 * @param node Inode.
 */
void print_inode(inode_t *node);

/**
 * Reads the given st_mode flags using the bitmasks from sys/stat.h
 * and sets any integers passed in as pointers to 1 if the respective
 * flag is set. NULL should be passed in for the ints of flags to not
 * check.
 *
 * @param mode Octal representing the st_mode file attributes.
 * @param isdirectory Flag representing directory.
 * @param isfile Flag representing a regular file.
 * @param canread Flag for user read permission.
 * @param canwrite Flag for user write permission.
 * @param canexecute Flag for user read permission.
 */
void read_mode(int mode, int *isdirectory, int *isfile, int *canread,
               int *canwrite, int *canexecute);

#endif
