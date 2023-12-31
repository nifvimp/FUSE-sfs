// Directory manipulation functions.
//
// Feel free to use as inspiration. Provided as-is.

// Based on cs3650 starter code
#ifndef DIRECTORY_H
#define DIRECTORY_H

// Maximum length of a directory entry name.
#define DIR_NAME_LENGTH 48

#include "blocks.h"
#include "inode.h"
#include "slist.h"

extern const int DIRENT_SIZE; // Size of dirent struct in bytes.
extern const int DIR_ROOT;    // Inode number of root directory.

typedef struct dirent_t {
  char name[DIR_NAME_LENGTH];
  int inum;
  char _reserved[12];
} dirent_t;

/**
 * Verifies the root directory and Initalizes it if it doesn't exist.
 */
void directory_init();

/**
 * Gets the inode number of the directory entry with the given
 * name in the given directory. Returns -1 if such an entry
 * does not exist.
 *
 * @param di Inode of the directory.
 * @param name Name of entry to look for.
 * @return The inode
 */
int directory_lookup(inode_t *di, const char *name);

/**
 * Copies the information of the nonempty directory entry of
 * the given index in the given directory to the given dirent
 * struct.
 *
 * @param di Inode of the directory.
 * @param dirent Dirent struct to copy to.
 * @param dnum Directory number (index).
 *
 * @return 0 on success, -1 on failure.
 */
int directory_read(inode_t *di, dirent_t *dirent, int dnum);

/**
 * Puts a new directory entry into the given directory with
 * the given name and inode number.
 *
 * @param di Inode of the directory.
 * @param name Name of the directory entry.
 * @param inum Inode number of the directory entry.
 *
 * @return 0 on sucess, -1 on failure.
 */
int directory_put(inode_t *di, const char *name, int inum);

/**
 * Deletes the directory entry with the given name in the
 * given directory.
 *
 * @param di Inode of the directory.
 * @param name The entry name.

 * @return 0 on success, -1 on failure.
 */
int directory_delete(inode_t *di, const char *name);

/**
 * Lists the names of all the nonempty directory entries
 * in the given directory.
 *
 * @param di Inode of the directory.
 *
 * @return List of names of directory entries in directory.
 */
slist_t *directory_list(inode_t *di);

/**
 * Prints the state of the given directory in the following format:
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
 *     entries:
 *       *entry name 1*
 *       *entry name 2*
 *       ...
 *
 * or prints 'N/A' if the given inode is not a directory.
 *
 * @param dd Inode of the directory
 */
void print_directory(inode_t *dd);

/**
 * Gets the inode of the file at the given path.
 *
 * @param path Path to file.
 * @return Pointer to the inode of the file at the given
 *         path, or NULL if the file cannot be found.
 */
inode_t *path_get_inode(const char *path);

#endif
