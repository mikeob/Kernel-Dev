#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
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
bool inode_check_permissions (struct inode *inode, int group, int flags);
uint8_t inode_get_permissions (struct inode *inode, int group);
bool inode_chmod (struct inode *inode, int group, uint8_t permissions);

#endif /* filesys/inode.h */
