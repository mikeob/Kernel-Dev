#include "filesys/cache.h"
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/off_t.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>



/* Cache is limited to 64 sectors */
#define CACHE_LIMIT = 64;
static struct cache_block * blocks[CACHE_LIMIT];

/* Definition for the cache block. Opaque to outsiders,
 * so we can assume any access to these fields is
 * done inside cache.c */
struct cache_block {
  block_sector_id bs_id;
  bool dirty;
  bool valid;
  int num_readers;
  int num_writers;
  int pending_requests;
  // Some sort of lock needed
  // Signal variables to signal availability changes
  // Usage info for eviction policy
  void * data;
};

struct block *fs_device;

/* Initializes our cache buffer */
void cache_init (void)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");


  // Initialize the cache_blocks
  int i;
  for (i = 0; i < CACHE_LIMIT; i++)
  {


  }

}

/* Reserves a block in the buffer cache dedicated to hold this sector
 * possibly evicting some other unused buffer. Either grants
 * exclusive or shared access */
struct cache_block * cache_get_block (disk_sector_t sector, bool exclusive)
{



}


/* Release access to a cache block */
void cache_put_block (struct cache_block *b)
{



}

/* Read cache block from the disk, returns pointer to the data */
void *cache_read_block (struct cache_block *b)
{




}


/* Zero a cache block and return pointer to the data */
void *cache_zero_block (struct cache_block *b)
{




}


/* Marks the dirty bit of a cache block */
void cache_mark_block_dirty (struct cache_block *b)
{
  b->dirty = true;
}


/* Eviction for the buffer cache 
 * Must write back upon eviction if the dirty bit is set. */
static void evict (void)
{


}
