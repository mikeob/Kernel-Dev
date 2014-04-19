#include "vm/frame.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "userprog/syscall.h"
#include "threads/synch.h"
#include "vm/page.h"
#include <stdio.h>
#include <debug.h>
#include <list.h>
#include <string.h>

static struct frame_entry * evict(void);
static struct lock f_lock;
static struct list used_list;   /* List of used frame_entries */
static struct list free_list;
static struct frame_entry * select_victim(void);
static struct list_elem *clock_position;

/* Initially the frame table is populated
 * with a frame for each page in physical memory
 * and an empty list of threads referring
 * to the physical pages. */
void frame_init(void)
{
  lock_init(&f_lock);
  list_init(&used_list);
  list_init(&free_list);

  void *new_vaddr;

  while ((new_vaddr = palloc_get_page(PAL_USER)) != NULL)
  {

    struct frame_entry *new = malloc(sizeof (struct frame_entry));
    new->kaddr = new_vaddr;
    new->pinned = false;
    lock_init(&new->frame_lock);

    list_push_back(&free_list, &new->elem);

  }
}

void 
frame_free (struct frame_entry *f)
{
  frame_table_lock ();

  list_remove(&f->elem);
  list_push_back(&free_list, &f->elem);

  frame_table_unlock ();
}

/*
 * Gets a physical frame for usage
 * by a user program. Possibly evicts a frame. 
 *
 * Similar in use as palloc_get_page(PAL_USER).
 * */
struct frame_entry *
frame_get(bool zero)
{
  /* Grab the frame to give away, possibly evicting */
  struct frame_entry *new = NULL;
  
  frame_table_lock ();
  if (list_empty(&free_list))
  {
    frame_table_unlock ();
    new = evict();
  }
  else 
  {
    struct list_elem *e = list_pop_front(&free_list);
    new = list_entry(e, struct frame_entry, elem);
    frame_table_unlock ();
  }

  lock_acquire(&new->frame_lock);

  new->thread = thread_current ();


  if (zero)
  {
    memset(new->kaddr, 0, PGSIZE);
  }

  lock_release(&new->frame_lock);
  
  frame_table_lock ();
  list_push_back(&used_list, &new->elem);
  frame_table_unlock ();

  return new;
}

static struct frame_entry * select_victim_random(void);
static struct frame_entry * select_victim(void);
/* 
 * Evicts a physical frame, moving it to swap space
 * and invalidating any and all page references to the
 * frame. */
static struct frame_entry *
evict(void)
{
  /* Choose frame to evict. */
  frame_table_lock ();
  struct frame_entry *frame = select_victim_random();
	struct page_entry *evicted_page = frame->spt_entry;

  lock_acquire(&frame->frame_lock);
  //TODO Pinning the frame temporarily
  //frame->pinned = true;

  
 
  /* Invalidate page in page directory and free the frame. */
  if (frame->thread->pagedir != NULL)
  {
    pagedir_clear_page(frame->thread->pagedir, evicted_page->pagenum);
  } 

  frame_table_unlock ();
	
  // Handle preserving the page's data
  switch (evicted_page->type)
  {
    case STACK:
    case DATA: // Always write to swap
          {
            evicted_page->swap_slot = swap_write(frame->kaddr);
            evicted_page->in_swap = true;
            break;
          }
    case FILE: // Only write if file is dirty
          {
            if (pagedir_is_dirty(frame->thread->pagedir,
                  evicted_page->pagenum))
            {
              filesys_lock ();
              file_write_at(evicted_page->file, 
                frame->kaddr, evicted_page->size, evicted_page->offset);
              filesys_unlock ();
            }
            break;
          }
    case CODE: // Do nothing
          break;
  } 
 
  
  /* Update the page entry to not point here anymore */
  evicted_page->f_entry = NULL;
  evicted_page->imported = false;
  
  
  //TODO SUPER inefficient
  lock_release(&frame->frame_lock);
  
  return frame;
}

/* Extremely basic eviction algorithm. Simply chooses
 * the first frame that isn't pinned in our used_list */
static struct frame_entry *
select_victim_random(void)
{
 	struct list_elem *e;
	for (e = list_begin (&used_list); e != list_end (&used_list);
	 		 e = list_next (e))
		{
			struct frame_entry *frame = list_entry (e, struct frame_entry, elem);
			if (!frame->pinned)
				{
					list_remove (e);
					return frame;
				}
		}
	return NULL;

}

static struct frame_entry *
select_victim(void)
{

	struct frame_entry *frame;
	while (true)
		{
			frame = list_entry (clock_position, struct frame_entry, elem);
			if (!pagedir_is_accessed (thread_current()->pagedir, frame->spt_entry->pagenum) && !frame->pinned)
				{
					struct list_elem *previous = clock_position;
			  	clock_position = list_end (&used_list) ? list_begin (&used_list) : list_next (clock_position);	
					list_remove (previous);							
					break;
				}
			pagedir_set_accessed (thread_current()->pagedir, frame->spt_entry->pagenum, false);
			clock_position = list_end (&used_list) ? list_begin (&used_list) : list_next (clock_position);
		}
	return frame;
}

/* Acquires the lock on the frame table */
void frame_table_lock (void)
{
  lock_acquire(&f_lock);
}

/* Releases the lock on the frame table */
void frame_table_unlock (void)
{
  lock_release(&f_lock);
}
