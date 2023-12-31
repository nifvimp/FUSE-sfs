// Disk storage abstracttion.
//
// Feel free to use as inspiration. Provided as-is.

// based on cs3650 starter code

#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "slist.h"

/**
 * Checks if the file at the given path exists in the file system.
 *
 * @param path Path to the file.
 *
 * @return 0 if the file exists, -1 if it does not exist.
 */
int storage_access(const char *path);

/**
 * Copies the information stored in the inode representing the
 * file at the given path into the given stat struct.
 *
 * @param path Path to the file.
 * @param st Stat struct.
 *
 * @return 0 on success, -1 on failure.
 */
int storage_stat(const char *path, struct stat *st);

/**
 * Reads the file at the given path to the given buffer.
 *
 * @param path Path to file.
 * @param buf Destination to read into.
 * @param size Number of bytes to read.
 * @param offset Byte to start reading from.
 *
 * @return Number of bytes read, or -1 if the read failed.
 */
int storage_read(const char *path, char *buf, size_t size, off_t offset);

/**
 * Writes the contents from the given buffer into the file
 * at the given path.
 *
 * @param path Path to file.
 * @param buf Source to write from.
 * @param size Number of bytes to write.
 * @param offset Byte to start writting at.
 *
 * @return Number of bytes written, or -1 if the write failed.
 */
int storage_write(const char *path, const char *buf, size_t size, off_t offset);

/**
 * Changes the size of the file at the given path to the
 * specified size.
 *
 * @param path Path to file.
 * @param size Size to truncate to in bytes.
 *
 * @return 0 on success, -1 on failure.
 */
int storage_truncate(const char *path, off_t size);

/**
 * Creates a new file at the given path with the given mode.
 *
 * @param path Path to create file at.
 * @param mode Mode to set file to.
 *
 * @return 0 on success, -1 on failure.
 */
int storage_mknod(const char *path, int mode);

/**
 * Unlinks/deletes the file at the given path.
 *
 * @param path Path ot file.
 * @return 0 on success, -1 on failure.
 */
int storage_unlink(const char *path);

/**
 * Deletes the directory at the given path if and only if
 * the directory is empty.
 *
 * @param path Path to directory.
 *
 * @return 0 on success, -1 on failure.
 */
int storage_rmdir(const char *path);

/**
 * Moves the file at the specified 'from' path to the
 * specified 'to' path.
 *
 * @param from Path to file to move.
 * @param to Destination path.
 *
 * @return 0 on success, -1 on failure.
 */
int storage_rename(const char *from, const char *to);

/**
 * Returns a list of the names of the files in the
 * directory at the given path.
 *
 * @param path Path to directory.
 * @return List of names of files in directory.
 */
slist_t *storage_list(const char *path);

#endif
