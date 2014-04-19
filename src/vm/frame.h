#ifndef VM_FRAME_H
#define VM_FRAME_H


#include <debug.h>
#include <list.h>
#include "threads/thread.h"
#include "threads/synch.h"
//#include "vm/page.h"

/* Frame table
 *
 * Holds entries corresponding to physical pages
 * of memory. Points to the page table(s) referring
 * to the particular portion of memory.
 *
 * Each entry holds a reference to the thread(s) (if any)
 * currently using the frame.
 * */

struct frame_entry
{
  struct list_elem elem;  /* For free/used lists */
  struct lock frame_lock;  /* Lock for exclusive access */
  void *kaddr;   /* Physical Address of this frame */
  struct thread *thread; /* Thread using this frame */
  struct page_entry *spt_entry; /* Pointer to SPT entry */
  bool pinned;    /* Is the frame pinned? */
};

/* Idea for extra credit:
 *
 * Instead of having a struct thread in the frame_entry,
 * we could have a thread pointer in each page_entry, pointing
 * back to the thread it is resident in. 
 *
 * This would let us just have a list of page_entries here,
 * pointing to all of the page_entries, thus threads, that
 * reference this frame         */

void frame_init (void);
struct frame_entry * frame_get (bool);
void frame_free (struct frame_entry *);
void frame_remove (struct frame_entry *f);

void frame_table_lock (void);
void frame_table_unlock (void);
#endif 
