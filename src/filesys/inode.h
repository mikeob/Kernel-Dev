#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include <list.h>
#include "filesys/off_t.h"
#include "devices/block.h"

struct bitmap;

void inode_init (void);
bool inode_create (block_sector_t, off_t, bool is_dir);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
bool inode_is_dir (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

/* File permission utilities */
int inode_get_uid (struct inode *inode);
int inode_get_gid (struct inode *inode);
//bool inode_check_permissions (struct inode *inode, int group, int flags);
uint8_t inode_get_permissions (struct inode *inode, int group);
bool inode_chmod (struct inode *inode, int group, uint8_t permissions);

/* On-disk inode.
 *
 * Implemented permissions. We had a lot
 * of extra space, so we didn't worry about saving space
 *
 *
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    bool is_dir;                        /* Is this a directory? */
    int user_id;                        /* User id who created the file */
    int group_id;                       /* Group id who created the file */
    uint8_t user_permission;            /* User permission mapping */
    uint8_t group_permission;           /* Group permission mapping */
    uint8_t other_permission;           /* Other permission mapping */
    bool set_uid;                       /* Set uid bit */
    bool set_gid;                       /* Set uid bit */
    unsigned magic;                     /* Magic number. */
    uint32_t unused[120];               /* Not used. */
  };


/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };



#endif /* filesys/inode.h */
