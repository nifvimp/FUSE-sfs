#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "directory.h"
#include "inode.h"
#include "slist.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// Checks if the file at the given path exists
// in the file system.
int storage_access(const char *path) {
  return NULL != path_get_inode(path);
}

// Copies the information stored in the inode representing the
// file at the given path into the given stat struct.
int storage_stat(const char *path, struct stat *st) {
  inode_t *node = path_get_inode(path);
  return inode_stat(node, st);
}

// Reads the file at the given path to the given buffer.
int storage_read(const char *path, char *buf, size_t size, off_t offset) {
  inode_t *node = path_get_inode(path);
  if (NULL == node) {
    return -1;
  }
  return inode_read(node, buf, offset, size);
}
// Writes the contents from the given buffer into the file
// at the given path.
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
  inode_t *node = path_get_inode(path);
  if (NULL == node) {
    return -1;
  }
  return inode_write(node, buf, offset, size);
}

// Changes the size of the file at the given path to the
// specified size.
int storage_truncate(const char *path, off_t size) {
  inode_t *node = path_get_inode(path);
  if (NULL == node) {
    return -1;
  }
  if (size > node->size) {
    if (-1 == grow_inode(node, size)) {
      return -1;
    }
  }
  if (size < node->size) {
    if (-1 == shrink_inode(node, size)) {
      return -1;
    }
  }
  return 0;
}

// Splits a file path into the directory containing the file and the filename itself.
// "hello/world/hi.txt" would be split into "hello/world" and "hi.txt"
// Assumes path_to_dir is as long as path and name is as long as DIR_NAME_LENGTH.
int path_split_strings(const char *path, char *path_to_dir, char *name) {
  slist_t *elems = slist_explode(path, '/');

  if (!elems) {
    return -1;
  }
  slist_t *tail = elems;
  int off = 0;
  path_to_dir[off++] = '/';
  while (NULL != tail->next) {
    for (int ii = 0; '\0' != tail->data[ii]; ++ii, ++off) {
      path_to_dir[off] = tail->data[ii];
    }
    tail = tail->next;

    //makes sure that only one slash in a row can happen, and that the last character is not a slash.
    if (path_to_dir[off - 1] != '/' && NULL != tail->next) {
      path_to_dir[off++] = '/';
    }
  }
  path_to_dir[off] = '\0';
  memcpy(name, tail->data, DIR_NAME_LENGTH);
  return 0;
}

// Creates a new file at the given path.
int storage_mknod(const char *path, int mode) {
  int path_length = strlen(path) + 1;
  char path_to_dir[MAX(path_length, DIR_NAME_LENGTH)];
  char name[DIR_NAME_LENGTH];
  path_split_strings(path, path_to_dir, name);

  int inum = alloc_inode(mode);
  if (inum < 0) {
    return -1;
  }
  inode_t *di = path_get_inode(path_to_dir);

  if (-1 == directory_put(di, name, inum)) {
    return -1;
  }
  return 0;
}

// Unlinks / deletes the file at the specified path.
int storage_unlink(const char *path) {
  int path_length = strlen(path) + 1;
  char path_to_dir[MAX(path_length, DIR_NAME_LENGTH)];
  char name[DIR_NAME_LENGTH];
  path_split_strings(path, path_to_dir, name);
  inode_t *di = path_get_inode(path_to_dir);
  if (NULL == di) {
    return -1;
  }
  return directory_delete(di, name);
}

// Removes the given directory if and only if the
// directory is empty.
// NOTE: Assumes path is to a directory.
int storage_rmdir(const char *path) {
  inode_t *di = path_get_inode(path);
  dirent_t dirent;
  //if directory_read returns >= 0, then there is at least one directory
  if ((directory_read(di, &dirent, 0)) >= 0) {
    return -1;
  }
  return storage_unlink(path);
}

// Moves the file from the given path to the other
// specified path.
int storage_rename(const char *from, const char *to) {
  int path_length_from = strlen(from) + 1;
  int path_length_to = strlen(to) + 1;
  int longest_len = MAX(path_length_from, path_length_to);
  char from_dir[MAX(DIR_NAME_LENGTH, longest_len)];
  char to_dir[MAX(DIR_NAME_LENGTH, longest_len)];
  char from_name[DIR_NAME_LENGTH];
  char to_name[DIR_NAME_LENGTH];
  path_split_strings(from, from_dir, from_name);
  path_split_strings(to, to_dir, to_name);

  inode_t *from_di = path_get_inode(from_dir);
  inode_t *from_fi = path_get_inode(from);
  if (NULL == from_fi) {
    return -1;
  }

  inode_t *to_di = path_get_inode(to_dir);

  //check if the from path contains a standard file
  int isfile = 0;
  read_mode(from_fi->mode, NULL, &isfile, NULL, NULL, NULL);

  // if the 'from' path leads to a file, and the 'to' path leads to a directory,
  // add the file onto the 'to' directory.
  if (isfile > 0 && (path_get_inode(to) != NULL)) {
    to_di = path_get_inode(to);
    strcpy(to_name, from_name);
  }
  // copies 'from' to 'to'.
  if (-1 == directory_put(to_di, to_name, from_fi->inum)) {
    return -1;
  }
  // deletes 'from'.
  if (-1 == directory_delete(from_di, from_name)) {
    return -1;
  }
  return 0;
}

// Lists directory entries in the directory at the
// given path.
slist_t *storage_list(const char *path) {
  inode_t *di = path_get_inode(path);
  return directory_list(di);
}
