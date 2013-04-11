/* Buffer cache interface */
#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H


#define EXCLUSIVE = 0x1;
#define SHARED = 0x2



struct cache_block; // Opaque type for the cache block


void cache_init (void);


struct cache_block * cache_get_block (disk_sector_t sector, bool excl);
void cache_put_block (struct cache_block *b);
void *cache_read_block (struct cache_block *b);
void *cache_zero_block (struct cache_block *b);
void cache_mark_block_dirty (struct cache_block *b);

/* TODO 1. Readahead
 *      2. Shutdown */


#endif
