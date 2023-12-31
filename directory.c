#include <string.h>

#include "directory.h"
#include "bitmap.h"
#include "blocks.h"
#include "inode.h"

const int DIRENT_SIZE = sizeof(dirent_t);
const int DIR_ROOT = 1;

// Verifies the root directory and Initalizes it if it doesn't exist
void directory_init() {
  // Checks if root directory already exists
  void *ibm = get_inode_bitmap();
  inode_t *node = get_inode(DIR_ROOT);
  int isdir = 0;
  read_mode(node->mode, &isdir, NULL, NULL, NULL, NULL);
  if (bitmap_get(ibm, DIR_ROOT) && isdir) {
    return;
  }

  // Initializes root directory (force overrides inode 1).
  bitmap_put(ibm, DIR_ROOT, 1);
  node->inum = DIR_ROOT;
  node->mode = 040755; // mode for a directory
}

// Copies in the dirent with the dirent at the given index.
// Returns 0 on success, and -1 on failure.
// NOTE: assumes the given inode represents a directory.
int dirent_read(inode_t *di, dirent_t *dirent, int idx) {
  // checks if read was successful (reads exactly size of dirent).
  if (DIRENT_SIZE != inode_read(di, (char *) dirent, idx * DIRENT_SIZE, DIRENT_SIZE)) {
    return -1;
  }
  return 0;
}

// Gets the index of the dirent with the given name, or -1 if
// a dirent with that name does not exist.
// NOTE: Assumes the given inode represents a directory.
int dirent_lookup(inode_t *di, const char *name) {
  // searches and reads entire file for dirents.
  for (int ii = 0; ii < di->size / DIRENT_SIZE; ++ii) {
    dirent_t dirent;
    dirent_read(di, &dirent, ii);
    if (strcmp(name, dirent.name) == 0) {
      return ii;
    }
  }
  return -1;
}

// Gets the inode number of the directory entry with the given
// name in the given directory.
int directory_lookup(inode_t *di, const char *name) {
  int idx = dirent_lookup(di, name);
  if (idx < 0) {
    return -1;
  }
  // reads the information of the found dirent.
  dirent_t dirent;
  if (0 != dirent_read(di, &dirent, idx)) {
    return -1;
  }
  return dirent.inum;
}

// reads the dnumth directory entry into the given dirent struct.
// returns -1 if dnum out of range (dirent is last in this case)
int directory_read(inode_t *di, dirent_t *dirent, int dnum) {
  int count = -1;
  dirent_t temp;
  // searches entire directory file.
  for (int ii = 0; count < dnum && ii < di->size / DIRENT_SIZE; ++ii) {
    // checks if directory entry is not empty.
    if (0 == dirent_read(di, &temp, ii) && 0 != temp.inum) {
      ++count;
    }
  }
  // copies temp into the given dirent struct.
  if (NULL != dirent) {
    memcpy(dirent, &temp, DIRENT_SIZE);
  }
  // checks if target dirent was reached.
  return count >= dnum ? 0 : -1;
}

// attempts to put inode in directory.
int directory_put(inode_t *di, const char *name, int inum) {
  inode_t *node = get_inode(inum);
  if (!inode_valid(di) || !inode_valid(node)) {
    return -1;
  }
  // finds the next empty directory entry in the directory.
  int offset = di->size;
  dirent_t dirent;
  for (int ii = 0; ii < di->size / DIRENT_SIZE; ii++) {
    dirent_read(di, &dirent, ii);
    if (dirent.inum == 0) {
      offset = ii * sizeof(dirent_t);
      break;
    }
  }
  // write a new directroy entry to the directory.
  memcpy(&dirent.name, name, DIR_NAME_LENGTH);
  dirent.inum = inum;
  inode_write(di, (char *) &dirent, offset, DIRENT_SIZE);
  ++node->links;
  return 0;
}

// Deletes the directory entry with the given name. Returns 0
// on success and -1 on failure.
int directory_delete(inode_t *di, const char *name) {
  if (!inode_valid(di)) {
    return -1;
  }
  // checks if a dirent with the given name exists.
  int idx = dirent_lookup(di, name);
  if (-1 == idx) {
    return -1;
  }
  dirent_t dirent;
  // reads the dirent with the given name.
  if (0 == inode_read(di, (char *) &dirent, idx * DIRENT_SIZE, DIRENT_SIZE)) {
    return -1;
  }
  int inum = dirent.inum;
  inode_t *node = get_inode(inum);
  if (!inode_valid(node)) {
    return -1;
  }
  // checks if inode can be freed or not.
  if (--(get_inode(inum)->links) <= 0) {
    free_inode(inum);
  }
  // deletes the directory entry.
  dirent.inum = 0;
  memset(dirent.name, 0, DIR_NAME_LENGTH);
  inode_write(di, (char *) &dirent, idx * DIRENT_SIZE, DIRENT_SIZE);
  return 0;
}

// Returns a list of the directory entries in the given directory.
slist_t *directory_list(inode_t *di) {
  int isdir = 0;
  read_mode(di->mode, &isdir, NULL, NULL, NULL, NULL);
  if (NULL == di || isdir) {
    return NULL;
  }
  slist_t *entries = NULL;
  // searches directory file for nonempty directory entries.
  for (int ii = 0; ii < di->size / DIRENT_SIZE; ii++) {
    dirent_t dirent;
    inode_read(di, (char *) &dirent, ii * DIRENT_SIZE, DIRENT_SIZE);
    if (dirent.inum != 0) {
      entries = slist_cons(dirent.name, entries);
    }
  }
  // reverses the list since list is constructed in reverse.
  entries = slist_reverse(entries);
  return entries;
}

// Prints information about the given directory.
// NOTE: Assumes dd is a directory.
void print_directory(inode_t *dd) {
  print_inode(dd);
  slist_t *entries = directory_list(dd);
  printf("entries:\n");
  while (NULL != entries) {
    printf("  %s\n", entries->data);
    entries = entries->next;
  }
}

// gets the inode of the file at the given path.
inode_t *path_get_inode(const char *path) {
  // starts search from root directory.
  inode_t *di = get_inode(DIR_ROOT);
  if (strcmp(path, "/") == 0) {
    return di;
  }
  // splits path into list of directory / file names.
  slist_t *list = slist_explode(path + 1, '/');
  slist_t *elem = list;
  // searches for each directory starting from
  // the root directory until it reaches the
  // desired directory entry.
  while (NULL != elem) {
    int inum = directory_lookup(di, elem->data);
    if (-1 == inum) {
      slist_free(list);
      return NULL;
    }
    di = get_inode(inum);
    elem = elem->next;
  }

  slist_free(list);
  return di;
}
