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

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);
static struct inode *path_to_file (char *path);
static struct dir *path_to_dir (char *path, char **filename);

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
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, false)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

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

  char *copy = (char *) malloc(strlen(name) + 1); 
  if (copy == NULL)
  {
    PANIC("Malloc failure");
  }

  strlcpy(copy, name, strlen(name) + 1);


  //struct dir *dir = dir_open_root ();
  //struct inode *inode = NULL;

  //if (dir != NULL)
  //  dir_lookup (dir, name, &inode);
  //dir_close (dir);

  struct inode *inode = path_to_file(copy); 
  free(copy);



  return inode != NULL ? file_open (inode) : NULL;
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

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
static struct dir *
path_to_dir (char *path, char **filename)
{

  struct dir *cur_dir;
  struct dir *ans;

  if (path[0] == '/')
  {
    cur_dir = dir_open_root ();
    path++;
  }
  else
  {
    cur_dir = thread_current ()->cur_dir;
  }

  ans = cur_dir;

  char *token, *save_ptr;
  struct inode *cur_inode;

  for (token = strtok_r (path, "/", &save_ptr); token != NULL;
      token = strtok_r (NULL, "/", &save_ptr))
  {
      // Verify file exists
      if (cur_dir == NULL || !dir_lookup(cur_dir, token, &cur_inode))
      {
          inode_close(cur_inode);
          dir_close(cur_dir);
          dir_close(ans);
          return NULL;
      }
      
      /* Maintain reference for our answer */
      *filename = token;
      dir_close(ans);
      ans = cur_dir;

      // cur_inode is a file. Either we're on the last token, or failure.
      if (!inode_is_dir(cur_inode))
      {
        cur_dir = NULL;
      }
      else
      {
        cur_dir = dir_open(cur_inode);
        inode_close(cur_inode);
      }

  }

  return ans;
}


/* Given a -MUTABLE- path, returns the inode associated
 * with the last file in the path.
 *
 * Will fail if any of the inodes along
 * the way do not exist, or if they are files.
 *
 * The caller is expected to close the provided inode.
 *
 * */
static struct inode *
path_to_file (char *path)
{

 struct dir *cur_dir;

 // Start at root
 if (path[0] == '/')
 {
    cur_dir = dir_open_root ();
    path++;
 }
 // Else relative path
 else
 {
    cur_dir = dir_reopen(thread_current ()->cur_dir);
 }

 char *token, *save_ptr;
 struct inode *cur_inode = NULL;

 for (token = strtok_r(path, "/", &save_ptr); token != NULL;
     token = strtok_r(NULL, "/", &save_ptr))
 {
    // If file doesn't exist in current directory
    if (cur_dir == NULL || !dir_lookup(cur_dir, token, &cur_inode))
    {
      dir_close(cur_dir);
      inode_close(cur_inode);
      return NULL;
    }

    dir_close(cur_dir);

    // cur_inode is a file. Either we're on the last token, or we fail. 
    if (!inode_is_dir(cur_inode))
    {
      cur_dir = NULL;
    }
    else
    {
      cur_dir = dir_open(cur_inode);
      inode_close(cur_inode);
    }
 }

 return cur_inode;
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
