#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <debug.h>
#include <list.h>
#include <bitmap.h>
#include "devices/block.h"
#include "vm/page.h"

/* Tracks in-use and free swap slots. */

struct swap_table
{
	struct bitmap *used_slots; /* Bitmap that holds used swap slots */
};

/* Initializes the swap table. Grabs the SWAP
 * block from the devices and initializes the
 * necessary data regarding the block. Then
 * fills up the table with the proper bitmaps. */

void swap_init (void);
void swap_free (int);
uint32_t swap_write (void *kaddr);
void swap_read (int swap_slot, void *);

#endif
