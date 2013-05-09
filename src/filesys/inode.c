#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include <stdio.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "filesys/cache.h"
#include "threads/malloc.h"
#include "threads/synch.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define N 122
#define INDEX_SIZE 128

struct index_block
{
  block_sector_t blocks[INDEX_SIZE];
};

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t direct[N];
    block_sector_t indirect;
    block_sector_t indirect_two;
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    bool is_dir;                        /* Is this a directory? */
    unsigned magic;                     /* Magic number. */
  };

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    struct lock lock;                   /* Lock on the inode */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    bool is_dir;                        /* True if inode represents directory */
    bool dirty;
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    off_t length;
  };


static block_sector_t
expand_by_one (void)
{
  block_sector_t free_sector;

  if (!free_map_allocate(1, &free_sector))
  {
    return 0;
  }

  struct cache_block *expansion = cache_get_block (free_sector, EXCLUSIVE);
  cache_zero_block(expansion);
  cache_mark_block_dirty(expansion);
  cache_put_block(expansion);

  return free_sector;
}


/* Given the blockno, we translate that to either the
 * direct, indirect, or doubly indirect block. If the
 * resulting block is "empty" (unallocated), we 
 * extend the file */
static block_sector_t
MLI_translate (const struct inode *inode, block_sector_t blockno)
{
  block_sector_t ans = 0;

  // Grab a reference to the inode_disk
  struct cache_block *cb = cache_get_block(inode->sector, SHARED); 
  struct inode_disk *disk = (struct inode_disk *) cache_read_block(cb);


  // If in the direct block
  if (blockno < N)
  {
    // Expansion
    if (disk->direct[blockno] == 0)
    {
      //printf("Single expansion!\n");
      // Obtain exclusive access to inode_disk
      cache_put_block(cb);
      cb = cache_get_block(inode->sector, EXCLUSIVE);
      disk = (struct inode_disk *) cache_read_block(cb);

      disk->direct[blockno] = expand_by_one ();
      cache_mark_block_dirty(cb);
    }
    

    ans = disk->direct[blockno];
    cache_put_block(cb);
  }
  // If in the indirect block
  else if (blockno < N + INDEX_SIZE)
  {
    blockno -= N;

    // Need to create indirect index block
    if (disk->indirect == 0)
    {
      // Obtain exclusive access to inode_disk
      cache_put_block(cb);
      cb = cache_get_block(inode->sector, EXCLUSIVE);
      disk = (struct inode_disk *) cache_read_block(cb);

      disk->indirect = expand_by_one();
      cache_mark_block_dirty(cb);
    }

    block_sector_t indirect_sector = disk->indirect;
    cache_put_block(cb);

    // If expansion failed
    if (indirect_sector == 0)
    {
      return 0;
    }

    // Load indirect block
    cb = cache_get_block(indirect_sector, SHARED);
    struct index_block *index = (struct index_block*) cache_read_block(cb); 

    if (index->blocks[blockno] == 0)
    {
      cache_put_block(cb);
      cb = cache_get_block(indirect_sector, EXCLUSIVE);
      index = (struct index_block *) cache_read_block (cb);

      index->blocks[blockno] = expand_by_one();
      cache_mark_block_dirty(cb);
    }

    ans = index->blocks[blockno];
    cache_put_block(cb);
  }
  // Otherwise, in double indirect
  else
  {
    blockno = blockno - N - INDEX_SIZE;
    int di_block_no = blockno / INDEX_SIZE;
    int di_offset = blockno % INDEX_SIZE;


    // Initialize double indirect block (FIRST IB)
    if (disk->indirect_two == 0)
    {
      cache_put_block(cb);
      cb = cache_get_block(inode->sector, EXCLUSIVE);
      disk = (struct inode_disk *) cache_read_block(cb);

      disk->indirect_two = expand_by_one();
      cache_mark_block_dirty(cb);
    }

    block_sector_t id_two_sector = disk->indirect_two;
    cache_put_block(cb);
    if (id_two_sector == 0)
    {
      return 0;
    }

    cb = cache_get_block(id_two_sector, SHARED);
    struct index_block *index = (struct index_block *) cache_read_block(cb);

    // Initialize double indirect block (SECOND IB)
    if (index->blocks[di_block_no] == 0)
    {
      cache_put_block(cb);
      cb = cache_get_block(id_two_sector, EXCLUSIVE);
      index = (struct index_block *) cache_read_block(cb);
     
      index->blocks[di_block_no] = expand_by_one();
      cache_mark_block_dirty(cb);
    }

    block_sector_t second_index = index->blocks[di_block_no];
    cache_put_block(cb);

    if (second_index == 0)
    {
      return 0;
    }

    cb = cache_get_block(second_index, SHARED); 

    index = (struct index_block *) cache_read_block(cb);

    // Initialize SECOND IB value
    if (index->blocks[di_offset] == 0)
    {
      cache_put_block(cb);
      cb = cache_get_block(second_index, EXCLUSIVE);
      index = (struct index_block *) cache_read_block(cb);


      index->blocks[di_offset] = expand_by_one();
      cache_mark_block_dirty(cb);
    }

    ans = index->blocks[di_offset];
    cache_put_block(cb);
  }

  return ans;
}

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  block_sector_t blockno = pos / BLOCK_SECTOR_SIZE;
  return MLI_translate(inode, blockno);
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_dir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      //size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->is_dir = is_dir;
      disk_inode->indirect = 0;
      disk_inode->indirect_two = 0;

      //printf("inode_create %u : disk_inode->direct[0] = %u\n", sector, disk_inode->direct[0]);

      // Write the disk_inode to disk
      struct cache_block *b = cache_get_block (sector, EXCLUSIVE); 
      void *buffer = cache_zero_block(b);
      memcpy(buffer, disk_inode, BLOCK_SECTOR_SIZE); 
      cache_mark_block_dirty(b);
      cache_put_block(b);

      success = true; 
      free (disk_inode);
    }

  if (length > 0)
  {
    struct inode *inode = inode_open (sector);
    inode_write_at(inode, "", 1, length - 1);
    inode_close(inode);
  }

  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  lock_init(&inode->lock);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  inode->dirty = false;


  struct cache_block *cb = cache_get_block(sector, SHARED);
  struct inode_disk *data = (struct inode_disk *) cache_read_block(cb);


  // Extract data from the on-disk inode
  inode->is_dir = data->is_dir;
  inode->length = data->length;
  cache_put_block(cb);


  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns the number of open instances of the inode exist */
int
inode_open_count (struct inode *inode)
{
  return inode->open_cnt;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Returns true if the inode represents a directory */
bool
inode_is_dir (const struct inode *inode)
{
  return inode->is_dir;
}

/* Frees all of the sectors in a Multi-Level Index file */
static void inode_free_data (struct inode *inode)
{
  struct cache_block *cb = cache_get_block(inode->sector, EXCLUSIVE);
  struct inode_disk *disk = (struct inode_disk *) cache_read_block(cb);

  int i, j;
  for (i = 0; i < N; i++)
  {
    if (disk->direct[i])
    {
      free_map_release(disk->direct[i], 1);
    }

  }

  block_sector_t indirect = disk->indirect;
  block_sector_t indirect_two = disk->indirect_two;
  cache_mark_deleted(cb);
  cache_put_block(cb);

  struct index_block *index;

  if (indirect)
  {
    cb = cache_get_block(indirect, EXCLUSIVE);
    index = (struct index_block *) cache_read_block(cb); 

    for (i = 0; i < INDEX_SIZE; i++)
    {
      if (index->blocks[i])
      {
        free_map_release(index->blocks[i], 1);
      }

    }

    free_map_release(indirect, 1);
    cache_mark_deleted(cb);
    cache_put_block(cb);
  }

  if (indirect_two)
  {
    cb = cache_get_block(indirect_two, EXCLUSIVE);
    index = (struct index_block *) cache_read_block(cb); 

    // For each indirect index
    for (i = 0; i < INDEX_SIZE; i++)
    {
      if (index->blocks[i])
      {
        struct cache_block *cb_index = cache_get_block(index->blocks[i], EXCLUSIVE);
        struct index_block *inner_index = 
          (struct index_block *) cache_read_block(cb_index);

        // For each inner index
        for (j = 0; j < INDEX_SIZE; j++)
        {
          if (inner_index->blocks[j])
          {
            free_map_release(inner_index->blocks[j], 1);
          }

        }

        cache_mark_deleted(cb_index);
        cache_put_block(cb_index);
      }
    }

    free_map_release(indirect_two, 1);
    cache_mark_deleted(cb);
    cache_put_block(cb);
  }

}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  
  /* Ignore null pointer. */
  if (inode == NULL)
    return;


  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed) 
      {
        inode_free_data (inode);
        free_map_release (inode->sector, 1);
      }
      else if (inode->dirty)
      {
        struct cache_block *cb = cache_get_block(inode->sector, EXCLUSIVE);
        struct inode_disk *disk = (struct inode_disk *) cache_read_block(cb);
        disk->length = inode->length;
        cache_mark_block_dirty(cb);
        cache_put_block(cb);
      }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{

  off_t bytes_read = 0;
  struct cache_block *cb;

  while (size > 0) 
    {

      // Don't allow expansion
      if (inode_length(inode) - offset <= 0)
        break;

      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;


      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;
          
      /* Read in data using buffer cache */ 
      cb = cache_get_block(sector_idx, SHARED);
      void * data = cache_read_block(cb);
      memcpy(buffer_ + bytes_read, data + sector_ofs, chunk_size); 
      cache_put_block(cb);
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  off_t bytes_written = 0;
  struct cache_block *cb;

  if (inode->deny_write_cnt)
    return 0;

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      
      //printf("byte_to_sector (%p, %d) = %u\n", inode, offset, sector_idx);
      // If file expansion fails
      if (sector_idx == 0)
      {
        break;
      }

      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      //off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      //int min_left = inode_left < sector_left ? inode_left : sector_left;
      int min_left = sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      void * data;

      cb = cache_get_block(sector_idx, SHARED);
      //printf("cache_get_block (%u, SHARED)\n", sector_idx);
      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
      {
        // If we're overriding the whole block, just zero the data
        data = cache_zero_block(cb);
      }
      else
      {
        data = cache_read_block(cb);
      }
      
      memcpy(data + sector_ofs, buffer_ + bytes_written, chunk_size);
      cache_mark_block_dirty(cb);
      cache_put_block(cb);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }

  
  // Actually mark the expansion
  if (!inode_is_dir(inode))
    lock_acquire(&inode->lock);

  if (inode->length < size + offset) 
  {
    inode->dirty = true;
    inode->length = size + offset;
  }

  if (!inode_is_dir(inode))
    lock_release(&inode->lock);

  return bytes_written;
}

void
inode_lock (struct inode *inode)
{
  lock_acquire(&inode->lock);
}

void
inode_release (struct inode *inode)
{
  lock_release (&inode->lock);
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->length;
}
