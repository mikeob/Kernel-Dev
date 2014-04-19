#include "vm/page.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void grab_from_swap(void *kpage, int swap_slot);

/* The necessary hashtable stuff
 * gets initialized here, but page_entries
 * are created on a need basis. */
void spage_init(void)
{
}

/* Fills out the given information in a supp. page entry
 * and inserts it into the hash. Doesn't do anything with it otherwise,
 * leaving most of the work to be done on page fault. */
struct page_entry * spage_create(struct hash *h,void *pagenum, 
    enum page_type type,
    struct file *f, off_t offset, uint32_t read_bytes, 
    uint32_t zero_bytes, bool writable)
{

  //TODO Patchwork solution to avoid duplicate creation

  if (spage_lookup(h, pagenum) != NULL)
  {
    PANIC("Duplicate creation attempted");
    return NULL;
  }
  
  struct page_entry *p = malloc(sizeof (struct page_entry));
  
  if (p == NULL)
  {
    PANIC("Malloc failure\n");
  }

  p->pagenum = pagenum;
  p->first_import = true;
  p->file = f;
  p->type = type;
  p->read_bytes = read_bytes;
  p->zero_bytes = zero_bytes;
  p->offset = offset;
  p->writable = writable;
  p->f_entry = NULL;
  p->in_swap = false;

  hash_insert(h, &p->hash_elem);

  return p;
}

/* Massive function that will cover the majority
 * of lazy loading. 
 *
 * When passed an address, the supp page entry
 * is looked up in the hash table, then the 
 * necessary files are loaded. */
bool spage_handle_fault (struct hash *h, void *vaddr)
{

  void * pagenum = pg_round_down(vaddr);


  // Obtain page_entry, return failure if it's not found
  struct page_entry *p_entry = spage_lookup(h, vaddr);

  if (p_entry == NULL)
  {
    //PAINC("spage_handle_fault failure\n");
    return false;
  }

  struct frame_entry *frame = p_entry->f_entry;
  // I faulted even though I should be present,
  // must be in transit
  while (frame)
  {
    lock_acquire(&frame->frame_lock); 
    struct frame_entry *new = p_entry->f_entry;
    lock_release(&frame->frame_lock);
    frame = new;
  }

  // Obtain a frame to store the page
  //TODO Always zeroing currently
  struct frame_entry *f_entry = frame_get(true);
  lock_acquire(&f_entry->frame_lock);
  
  p_entry->f_entry = f_entry;
  f_entry->spt_entry = p_entry;
  
  uint8_t * kpage = f_entry->kaddr;


  // Locate the data
  switch (p_entry->type)
  {
    case DATA: // READ/WRITE. Initially from exec, then from swap 
               // Page out: swap
      { 
        if (!p_entry->first_import) 
        {
          grab_from_swap(kpage, p_entry->swap_slot);
          p_entry->in_swap = false;
          break;
        }

      }
    case CODE:  // Code from the executable. Always pull from file
    case FILE:
      {

        uint32_t read_bytes = p_entry->read_bytes;
        uint32_t zero_bytes = p_entry->zero_bytes;
        
        
        //TODO Do we need to lock the file system?
        if (file_read_at (p_entry->file, kpage, read_bytes, p_entry->offset)
            != (int) read_bytes)
        {
          frame_free(f_entry);
          return false;
        }
        // Zero out the remaining
        if (zero_bytes) 
        {
          memset (kpage + read_bytes, 0, zero_bytes);
        }
        break;
      }

    case STACK: // Stack expansion. 

      
      // If first import, just grab the data - no zeroing necessary. 
      // otherwise, must grab from swap
      if (!p_entry->first_import)
      {
        grab_from_swap(kpage, p_entry->swap_slot);
        p_entry->in_swap = false;
      }
      break;

  }
  // Mark the page table as active
  if (!pagedir_set_page (thread_current ()->pagedir, 
        pagenum, kpage, p_entry->writable))
  {
    frame_free(f_entry);
    return false;

  }

  if (p_entry->first_import)
  {
    p_entry->first_import = false;
  }

  //TODO Pinning the page temporarily
  //f_entry->pinned = false;

  lock_release(&f_entry->frame_lock);

  return true;
}

/* Writes the page from swap located at sector swap_slow
 * to kpage, then frees the particular swap page */
static void grab_from_swap(void *kpage, int swap_slot)
{
  swap_read(swap_slot, kpage);
  swap_free(swap_slot);
}

/* Accesses the hash table of page_entries and returns the
 * page_entry corresponding to the page_addr */
struct page_entry *
spage_lookup (struct hash *h, const void *vaddr)
{

  void * pagenum = pg_round_down(vaddr);

  struct page_entry p;
  struct hash_elem *e;

  p.pagenum = pagenum;
  e = hash_find (h, &p.hash_elem);
  return e != NULL ? hash_entry(e, struct page_entry, hash_elem) : NULL;
}

/* Used for erasing allocated pages when unmapping
 * a mmap'd file. */
void
spage_erase (struct page_entry *p)
{
  if (p->f_entry != NULL)
  {
    lock_acquire(&p->f_entry->frame_lock);
    frame_free(p->f_entry);
    lock_release(&p->f_entry->frame_lock);
  }
  hash_delete (&thread_current ()->spt_hash, &p->hash_elem);
  free (p);
}

/* Used in process.c to set up the first
 * page in the new stack. This page cannot be lazily
 * loaded, so it merits its own function */
bool spage_setup_stack (void *pagenum)
{
  struct page_entry *spe = spage_create (&thread_current ()->spt_hash,
      pagenum, STACK, NULL, 0, 0, 0, true);

  if (spe == NULL)
  {
    return false;
  }

  //printf("Setup stack page_entry address: %p\n", spe);
  /*
  struct frame_entry *f = frame_get(true);
  spe->f_entry = f;
  //TODO point of contention here
  spe->first_import = false;
  spe->imported = true;
  f->spt_entry = spe;

  if (pagedir_get_page (thread_current()->pagedir, pagenum) != NULL
      || !pagedir_set_page (thread_current()->pagedir, pagenum, f->kaddr, true))
  {
    frame_free(f);
    return false;
  }
  */ 
  return spage_handle_fault(&thread_current()->spt_hash, pagenum);
  //return true;
}

//TODO Write a function that forces the population
//      of a supp page entry
void spage_touch (void * pagenum)
{


}

// Take the address. If we can find a page associated with it in our spt,
// then all we need to do is import. If we don't have a page, run it 
// through the tests? (ugh, i hope not), create the page, then import it.
// Then pinning needs to be done, but this happens in syscall.c.

/*--------- Hash Functions Defined ------------ */
unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct page_entry *p = hash_entry(p_, struct page_entry, hash_elem);
  return hash_bytes (&p->pagenum, sizeof p->pagenum);
}

bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux UNUSED)
{
  const struct page_entry *a = hash_entry(a_, struct page_entry, hash_elem);
  const struct page_entry *b = hash_entry(b_, struct page_entry, hash_elem);

  return a->pagenum < b->pagenum;
}

void page_destroy(struct hash_elem *elem, void *aux)
{
  //TODO Possibly remove the aux argument and change
  // process_exit to use pagedir_destroy()
  struct page_entry *p = hash_entry(elem, struct page_entry, hash_elem);

  //printf("Attempting to free %p with frame %p\n", p, p->f_entry);
  
  frame_table_lock ();
  if (p->f_entry != NULL)
  {
    lock_acquire(&p->f_entry->frame_lock);
    frame_table_unlock ();
    frame_free(p->f_entry);
    lock_release(&p->f_entry->frame_lock);
  }
  else 
  {
    frame_table_unlock ();
  }
  
  /* Free swap space, if being used */
  /*if (p->in_swap)
  {
    swap_free(p->swap_slot);
  }*/
  free(p);
}


