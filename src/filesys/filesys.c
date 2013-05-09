#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "threads/malloc.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/cache.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  cache_flush ();
  free_map_flush ();
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{

  //printf("create(%s)\n", name);
  // Don't accept empty string
  if (strlen(name) == 0)
  {
    return false;
  }

  char *copy = (char *) malloc(strlen(name) + 1); 
  if (copy == NULL)
  {
    PANIC("Malloc failure");
  }

  strlcpy(copy, name, strlen(name) + 1);

  char *filename;

  block_sector_t inode_sector = 0;
  struct dir *dir = filesys_path_to_dir(copy, &filename);

  
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, false)
                  && dir_add (dir, filename, inode_sector));

  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  free(copy);

  return success;
}

/* Given an absolute or relative path,
 * changes the current directory to the 
 * corresponding directory.
 *
 * Fails upon empty path, non-existant directory,
 * or directory is just a file */
bool 
filesys_chdir (const char *name)
{


  if (strlen(name) == 0)
  {
    return false;
  }

  char *copy = (char *) malloc(strlen(name) + 1); 
  if (copy == NULL)
  {
    PANIC("Malloc failure");
  }

  strlcpy(copy, name, strlen(name) + 1);

  struct inode *inode;
  char *filename;

  struct dir *dir = filesys_path_to_dir(copy, &filename);


  if (dir == NULL)
  {
    return false;
  }

  if (dir_lookup(dir, filename, &inode) && inode_is_dir(inode))
  {
    struct dir *old = thread_current ()->cur_dir;
    thread_current ()->cur_dir = dir_open(inode);
    dir_close(dir);
    dir_close(old);
    return true;
  }



  dir_close (dir);

  return false;
}

bool
filesys_mkdir (const char *name)
{
  //printf("mkdir (%s)\n", name);
  if (strlen(name) == 0)
  {
    return false;
  }

  char *copy = (char *) malloc(strlen(name) + 1); 
  if (copy == NULL)
  {
    PANIC("Malloc failure");
  }

  strlcpy(copy, name, strlen(name) + 1);

  struct inode *inode;
  char *filename;

  block_sector_t inode_sector = 0;
  struct dir *dir = filesys_path_to_dir(copy, &filename);


  if (dir == NULL)
  {
    return false;
  }

  // If something already exists with that name
  if (dir_lookup(dir, filename, &inode))
  {
    dir_close(dir);
    inode_close(inode);
    return false;
  }

  bool success = (free_map_allocate (1, &inode_sector)
                  && dir_create(inode_sector, 16)
                  && dir_add(dir, filename, inode_sector));


  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);


  dir_close (dir);
  free(copy);

  return success;

}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  //printf("filesys_open(%s)\n", name);

  char *copy = (char *) malloc(strlen(name) + 1); 
  if (copy == NULL)
  {
    PANIC("Malloc failure");
  }
  char *filename;

  strlcpy(copy, name, strlen(name) + 1);

  struct dir *dir = filesys_path_to_dir(copy, &filename); 

  if (dir == NULL)
  {
    return NULL;
  }

  
  struct inode *inode;
  if (!dir_lookup(dir, filename, &inode))
  {
    //printf("DIR_LOOKUP FAILURE IN FILESYS_OPEN\n");
  }
  dir_close (dir);
  free(copy);
  //printf("Filesys_open given inode %p\n", inode);
  //printf("inode sector is %u\n", inode_get_inumber(inode));

  return inode != NULL ? file_open (inode) : NULL;
}


/* Deletes the file or empty directory named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  //printf("remove %s\n", name);

  char *copy = (char *) malloc(strlen(name) + 1); 
  if (copy == NULL)
  {
    PANIC("Malloc failure");
  }

  strlcpy(copy, name, strlen(name) + 1);
  char *filename;

  struct dir *dir = filesys_path_to_dir (copy, &filename);

  bool success = dir != NULL && dir_remove (dir, filename);
  dir_close (dir); 
  free(copy);
  return success;
}

/* Given a -MUTABLE- path, returns the directory 
 * that contains the last file in the path.
 *
 * Used for file/directory creation.
 *
 * Requires that it is given a mutable string,
 * and passes the filename of the final path entry
 * back to the caller. On failure, returns NULL.
 *
 * */
struct dir *
filesys_path_to_dir (char *path, char **filename)
{

  struct dir *cur_dir;

  /* Special case for path == "/" */
  if (!strcmp(path, "/"))
  {
    *filename = ".";
    return dir_open_root ();
  }

  if (path[0] == '/')
  {
    cur_dir = dir_open_root ();
    path++;
  }
  else
  {
    cur_dir = dir_reopen(thread_current ()->cur_dir);
  }


  char *token, *save_ptr;
  struct inode *cur_inode;

  token = strtok_r (path, "/", &save_ptr);

  if (token == NULL)
  {
    dir_close(cur_dir);
    return NULL;
  }

  while (true)
  {
    dir_lookup (cur_dir, token, &cur_inode);
    *filename = token;
    token = strtok_r (NULL, "/", &save_ptr); 

    // Nothing left to parse, success!
    if (token == NULL)
    {
      inode_close(cur_inode);
      break;
    }

    // Failure, because we have more to parse but cannot descent
    // deeper
    if (cur_inode == NULL || !inode_is_dir(cur_inode))
    {
      dir_close(cur_dir);
      inode_close(cur_inode);
      return NULL;
    }

    struct dir *old = cur_dir;
   
    // Descend 
    //dir_close(cur_dir);
    cur_dir = dir_open(cur_inode);

    dir_close(old);

  }

  return cur_dir;
}



/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
