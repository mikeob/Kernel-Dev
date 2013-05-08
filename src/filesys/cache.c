#include "filesys/cache.h"
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/off_t.h"
#include "threads/synch.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>



/* Cache is limited to 64 sectors */
#define CACHE_LIMIT 64
#define BLOCK_SIZE 512
static struct cache_block blocks[CACHE_LIMIT];

/* Definition for the cache block. Opaque to outsiders,
 * so we can assume any access to these fields is
 * done inside cache.c */
struct cache_block {
  block_sector_id bs_id;        /* Indicates the block sector. 0 if unused */
  bool dirty;
  bool valid;
  int num_readers;
  int num_writers;
  int pending_requests;         /* Number of pending read/write requests */
  struct lock lock;
  struct condition cond_write;  /* Condition variable for writers */
  struct condition cond_read;   /* Condition variable for readers */
  //TODO Usage info for eviction policy
  void * data;
};

struct block *fs_device;

/* Initializes our cache buffer */
void cache_init (void)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize buffer cache.");


  // Initialize the cache_blocks
  int i;
  for (i = 0; i < CACHE_LIMIT; i++)
  {
      blocks[i].data = malloc(BLOCK_SIZE);
      if (blocks[i].data == NULL)
      {
        PANIC("Malloc failure in cache_init");
      }

      lock_init(&blocks[i].lock);
      cond_init(&blocks[i].cond_write);
      cond_init(&blocks[i].cond_read);
      init_block(&blocks[i], 0);
  }
}

/* If the block is valid and dirty, writes it back
 * to disk. 
 *
 * Assumes lock on block is held! */
static void
writeback(struct cache_block *b)
{

  // Error checking!
  if (b->dirty && !b->valid)
  {
    PANIC("Dirty but not valid!");
  }

  if (b->valid && b->dirty)
  {
    block_write(fs_device, b->bs_id, b->data)
    b->dirty = false;
  }
}


/* Calls writeback on the entire cache, 
 * requiring any dirty and valid blocks to
 * be written back to disk */
void cache_flush (void)
{
  int i;
  for (i = 0; i < CACHE_LIMIT; i++)
  {
    lock_acquire(&blocks[i].lock);
    writeback(blocks[i]);
    lock_release(&blocks[i].lock);
  }
}


/* Initializes a block to a fresh state. 
 *
 * Assumes the lock on the block is held upon calling
 * init_block */
static void
init_block(struct cache_block *b, disk_sector_t sector)
{
  b->bs_id = sector;
  b->valid = false;
  b->dirty = false;
  b->num_readers = 0;
  b->num_writers = 0;
  b->pending_requests = 0;
}
 
/* Attempts to secure a block for the provided sector.
 *
 * First checks to see if that sector already has a loaded
 * block.
 *
 * Second, evicts a block and initalizes one for use */
static struct cache_block *
obtain_block (disk_sector_t sector)
{

  // Check to see if block already here!
  int i;
  for (i = 0; i < CACHE_LIMIT; i++) {
    lock_acquire(&blocks[i].lock);
    if (blocks[i].bs_id == sector)
      return &blocks[i];
    lock_release(&blocks[i].lock);
  }
  
  return evict();
}

/* Evicts a block, writing its data back to disk if 
 * the dirty bit is set. Initially uses a random selection algorithm 
 * 
 * Returns the index of the block evicted, and holds the lock through
 * return.*/
static struct cache_block *
evict (void)
{
  struct cache_block *b = NULL;

  // Attempt to find an evictable block
  int i;
  for (i = 0; i < CACHE_LIMIT; i++)
  {
    b = blocks[i];
    lock_acquire(&b->lock);
    if (b->num_readers == 0 && b->num_writers == 0 && b->pending_requests == 0)
    {
      break;
    }
    lock_release(&b->lock);

    b = NULL;
  }

  // If failure
  if (b == NULL)
  {
    //TODO Possibly just loop evict until success?
    PANIC("Evict failure in cache_block");
  }

  writeback(b);
  init_block(b, sector);

  return b;
}

/* Reserves a block in the buffer cache dedicated to hold this sector
 * possibly evicting some other unused buffer. Either grants
 * exclusive or shared access */
struct cache_block * cache_get_block (disk_sector_t sector, bool exclusive)
{
  
  struct cache_block *b = obtain_block(sector);

  // No need to acquire lock - we already have it!

  b->pending_requests++;

  if (exclusive)
  {
    while (b->num_readers > 0 || b->num_writers > 0)
      cond_wait(&b->cond_write, &b->lock);

    b->num_writers++;
  }
  else
  {
    while (b->num_writers > 0)
      cond_wait(&b->cond_read, &b->lock);

    b->num_readers++;
  }

  b->pending_requests--;

  lock_release(&b->lock);
  return b;
}


/* Release access to a cache block */
void cache_put_block (struct cache_block *b)
{
  lock_acquire(&b->lock);

  if (b->num_writers > 0)
  {
    b->num_writers--;
    assert(b->num_writers == 0);

    // Signal others!
    cond_broadcast(&b->cond_read, &b->lock);
    cond_signal(&b->cond_write, &b->lock);
  }
  else if (--b->num_readers == 0)
  {
    cond_signal(&b->cond_write, &b->lock);
  }
  // Otherwise, others are reading, and do nothing!
  else
  {
    lock_release(&b->lock);
    return;
  }

  //TODO If no requests pending, put up for eviction
  if (b->pending_requests == 0)
  {
  }

  lock_release(&b->lock);
}

/* Read cache block from the disk, returns pointer to the data */
void *cache_read_block (struct cache_block *b)
{
  lock_acquire(&b->lock);

  // If the data isn't loaded, load it
  if (!b->valid)
  {
    block_read(fs_device, b->bs_id, b->data);
    b->valid = true;
  }
  
  lock_release(&b->lock);
  return b->data;
}


/* Zero a cache block and return pointer to the data */
void *cache_zero_block (struct cache_block *b)
{
  lock_acquire(&b->lock);
  memset(b->data, 0, BLOCK_SIZE);
  b->valid = true;
  lock_release(&b->lock);

  return b->data;
}


/* Marks the dirty bit of a cache block */
void cache_mark_block_dirty (struct cache_block *b)
{
  lock_acquire(&b->lock);
  b->dirty = true;
  lock_release(&b->lock);
}


