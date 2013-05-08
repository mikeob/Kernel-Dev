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
static struct inode *path_to_inode (char *path);
static bool verbose = false;

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

	char * shell = "shell";
	char * sudo = "sudo";
	char * chmod = "chmod";
  char * echo = "echo";
	char * passwd = "passwd";
  char * touch = "touch";

	if (!strcmp (name, shell))
		filesys_chmod (name, false, false, 7, 4, 1);

	if (!strcmp (name, sudo))
		filesys_chmod (name, true, false, 7, 4, 0);	 

	if (!strcmp (name, chmod))
		filesys_chmod (name, false, false, 7, 4, 1);

	if (!strcmp (name, echo))
		filesys_chmod (name, false, false, 7, 4, 1);

	if (!strcmp (name, passwd))
		filesys_chmod (name, true, false, 7, 4, 1);

  if (!strcmp (name, touch))
		filesys_chmod (name, false, false, 7, 4, 1);

	return success;
}

bool
filesys_chmod (const char *name, bool setuid, bool setgid, uint8_t user, uint8_t group, uint8_t others)
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
  bool success = false;

  if (dir_lookup(dir, filename, &inode))
  {
		if (setuid)
			inode_chmod(inode, FILE_SETUID, 0);
		if (setgid)
			inode_chmod(inode, FILE_SETGID, 0);
    inode_chmod(inode, FILE_USER, user);
    inode_chmod(inode, FILE_GROUP, group);
    inode_chmod(inode, FILE_OTHER, others);
    success = true;
  }

  return success;
}


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
    dir_close(thread_current()->cur_dir);
    thread_current ()->cur_dir = dir_open(inode);
    return true;
  }

  return false;
}

bool
filesys_mkdir (const char *name)
{
  if (verbose) {printf("filesys_mkdir(%s)\n", name);}
  if (strlen(name) == 0)
  {

    if (verbose) {printf("Failing mkdir, non-existant path\n");}

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
    if (verbose) { printf("filesys_path_to_dir failed\n"); }
    return false;
  }

  // If something already exists with that name
  if (dir_lookup(dir, filename, &inode))
  {
    if (verbose) { printf("File/directory already exists\n"); }
    dir_close(dir);
    inode_close(inode);
    return false;
  }

  if (verbose) {printf("filename = %s\n", filename);}
  bool success = (free_map_allocate (1, &inode_sector)
                  && dir_create(inode_sector, 16)
                  && dir_add(dir, filename, inode_sector));
/*
  bool success = true;

  if (!free_map_allocate (1, &inode_sector))
  {
    printf("free_map_allocate fail\n");
     success = false;
  }
  if (!dir_add(dir, filename, inode_sector))
  {
    printf("dir_add fail!\n");
    success = false;
  }

  //printf("free sector is %d\n", inode_sector);
  if (!dir_create(inode_sector, 16))
  {
    printf("dir_create fail\n");
    success = false;
  }

*/
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);

  dir_close (dir);
  free(copy);

  if (verbose && success) {printf("mkdir success!\n");}

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
  char *filename;

  strlcpy(copy, name, strlen(name) + 1);

  struct dir *dir = filesys_path_to_dir(copy, &filename); 

  if (dir == NULL)
  {
    return NULL;
  }
  
  struct inode *inode;
  dir_lookup(dir, filename, &inode);

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

  if (path[0] == '/')
  {
    cur_dir = dir_open_root ();
    path++;
    //printf("dir_open_root is %p\n", cur_dir);
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
    //printf("DESCENDING! cur_dir = %p\n", cur_dir);
    //dir_close(cur_dir);
    cur_dir = dir_open(cur_inode);
    //printf("cur_dir is %p\n", cur_dir);

    dir_close(old);

  }

  return cur_dir;
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
path_to_inode (char *path)
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
