#include <list.h>
#include <string.h>
#include <stdio.h>
#include <bitmap.h>
#include <debug.h>
#include "vm/swap.h"
#include "threads/synch.h"
#include "devices/block.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

#define SECTORS_PER_PAGE 8

/* Block operations
 *
 * void block_read (struct block *, block_sector_t, void *);
 * void block_write (struct block *, block_sector_t, const void *);
 * */

struct swap_table table;
struct block *swap_blk;
struct lock bit_lock;

/* Initializes swap.c
 *
 * Gets a reference to the swap block,
 * populates the list of bitmaps */
void swap_init (void)
{
  swap_blk = block_get_role (BLOCK_SWAP);

  block_sector_t size = block_size (swap_blk);
	size /= SECTORS_PER_PAGE; //8 sectors make a page
	table.used_slots = bitmap_create (size);

  lock_init(&bit_lock);
}

void 
swap_free (int swap_slot)
{
  lock_acquire(&bit_lock);
	bitmap_reset (table.used_slots, swap_slot);
  lock_release(&bit_lock);
}

uint32_t
swap_write (void * kaddr)
{
  lock_acquire(&bit_lock);
	uint32_t next_slot = bitmap_scan_and_flip(table.used_slots, 0, 1, false);

	// Panic the kernel if swap is full
	if (next_slot == BITMAP_ERROR)
	{
		PANIC ("Swap block full");	
	}
	
  
  /* A page takes up 8 sectors, so convert next_slot to sector (next*8) */
	block_sector_t sector = next_slot * SECTORS_PER_PAGE;
	int i;
	for (i = 0; i < SECTORS_PER_PAGE; i++) 
	{
		block_write (swap_blk, sector + i, kaddr + i*BLOCK_SECTOR_SIZE);
	}
  
  lock_release(&bit_lock);

  return next_slot;
}

void
swap_read (int swap_slot, void *buffer) 
{ 
  lock_acquire(&bit_lock);
  block_sector_t sector = swap_slot * SECTORS_PER_PAGE;
	int i;
	for (i = 0; i < SECTORS_PER_PAGE; i++)
	{
		block_read (swap_blk, sector + i, buffer + i*BLOCK_SECTOR_SIZE);
  }
  lock_release(&bit_lock);
}
