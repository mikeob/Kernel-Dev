#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <debug.h>
#include <list.h>
#include <hash.h>
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/synch.h"
#include "filesys/filesys.h"

/* Supplemental Page Table
 *
 * Provides additional information to the
 * page table.
 *
 * Two main purposes:
 * 1. On page fault, kernel looks up the virtual page
 *    in this page to determine what data should be there
 *    and where it should be retreived from.
 *
 * 2. When a process terminates, the kernel consults this
 *    to determine what resources to free.
 *
 * */


/* Tasks involving the SPT
 *
 *  Modify userprog/exception.c to do:
 *
 *  1. Locate the page that faulted in the SPT. If the memory
 *     address is valid, use the SPT to locate the data that
 *     should be there. Could be Filesystem, swap, or 0's.
 *
 *     If sharing is implemented, could already be in a frame.
 *
 *   
 *  2. Obtain a frame to store the page (via the frame table)
 *
 *  3. Fetech the data into the frame
 *
 *  4. Point the page table entry to the physical page. See 
 *     userprog/pagedir.c for functions to do this.
 *
 *
 * */


/* The different types of pages to reside
 * within the supplemental page table. */
enum page_type
{
  CODE,
  DATA,
  STACK,
  FILE
};


struct page_entry
{
  struct hash_elem hash_elem;
  void * pagenum;     /* The page number, or in other words the
                          page aligned virtual address
                          NOTE: This is the user address */
  uint32_t size;      /* The number of bytes the page actually uses */
  enum page_type type;
  bool imported;      /* Has the data been imported? */
  bool first_import;  /* Is this the first time we import it? */
  bool writable;     /* Read/write? */
  struct frame_entry *f_entry;/* Points to the frame that 
                                this page resides in
                                if any at all*/
  uint32_t read_bytes;
  uint32_t zero_bytes;
  
  struct file *file; /* The file for which the executable should be loaded
                        from. */
  off_t offset;      /* The offset from which we should begin reading */

  uint32_t swap_slot; /* If this page resides in swap, this is the slot number*/
  bool in_swap;
  struct list_elem map_elem; /* Used for tracking mmap */
};


/* Public functions */
void spage_init(void);
struct page_entry * spage_create(struct hash *h,
    void *pagenum, enum page_type type,
    struct file *f, off_t offset, uint32_t read_bytes, uint32_t zero_bytes,  bool writeable);
bool spage_handle_fault (struct hash *h, void *);
struct page_entry *spage_lookup (struct hash *h, const void *vaddr);
void spage_erase (struct page_entry *p);
bool spage_setup_stack (void * pagenum);


void page_destroy(struct hash_elem *elem, void *aux);
unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux UNUSED);
#endif
