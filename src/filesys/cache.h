/* Buffer cache interface */
#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"


#define EXCLUSIVE true
#define SHARED false



//struct cache_block; // Opaque type for the cache block


void cache_init (void);


struct cache_block * cache_get_block (block_sector_t sector, bool excl);
void cache_put_block (struct cache_block *b);
void *cache_read_block (struct cache_block *b);
void *cache_zero_block (struct cache_block *b);
void cache_mark_block_dirty (struct cache_block *b);
void cache_mark_deleted (struct cache_block *b);

void cache_flush (void);

block_sector_t cache_get_sector (struct cache_block *b);

/* TODO 1. Readahead
 *      2. Shutdown */


#endif
