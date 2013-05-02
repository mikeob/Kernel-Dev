#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */


/* File permission definitions */
#define FILE_USER 2
#define FILE_GROUP 4
#define FILE_OTHER 8
#define FILE_SETUID 16
#define FILE_SETGID 15

#define FILE_READ 4
#define FILE_WRITE 2
#define FILE_EXEC 1

/* Block device that contains the file system. */
struct block *fs_device;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size);
bool filesys_mkdir (const char *name);
bool filesys_chdir (const char *name);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);
struct dir *filesys_path_to_dir(char *path, char **filename);
bool filesys_chmod (const char *name, bool setuid, bool setgid, uint8_t user, uint8_t group, uint8_t others);


#endif /* filesys/filesys.h */
