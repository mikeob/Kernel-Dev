#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include <string.h>
#include "filesys/file.h"
#include "filesys/inode.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);
static void* syscall_read_stack (struct intr_frame *f, int locale);
void syscall_exit (int status);
static struct lock file_lock;
static void check_pointer(void *p);
static struct file_descriptor* check_fd (int fd);
static void close_fd (int fd);
static struct lock exiting_lock;


/* SYS CALL helper methods */
int sys_exec (struct intr_frame *f);
bool sys_create (struct intr_frame *f);
bool sys_remove (struct intr_frame *f);
int sys_open (struct intr_frame *f);
int sys_filesize (struct intr_frame *f);
int sys_read (struct intr_frame *f);
int sys_write (struct intr_frame *f); 
int sys_wait (struct intr_frame *f);
void sys_seek (struct intr_frame *f);
unsigned sys_tell (struct intr_frame *f);
void sys_close (struct intr_frame *f);
bool sys_chdir (struct intr_frame *f);
bool sys_mkdir (struct intr_frame *f);
bool sys_readdir (struct intr_frame *f);
bool sys_isdir (struct intr_frame *f);
int sys_inumber (struct intr_frame *f);



void syscall_init (void) 
{
	lock_init (&file_lock);	
  lock_init (&exiting_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
	/* f->esp holds the system call pointer. Return values should be placed
	   f->eax. Only syscall instruction for project 2 are incluced */
	int sys_call = *(int *)syscall_read_stack (f, 0);
  switch (sys_call) 
	{
		case SYS_HALT: // void halt (void)
      { 
				shutdown_power_off();
			  break;
      }
		case SYS_EXIT: // void exit (int status)
			{
				int status = *(int *)syscall_read_stack(f, 1);
				syscall_exit (status);
				break;
			}
		case SYS_EXEC: // pid_t exec (const char *cmd_line)
      f->eax = sys_exec(f);
      break;
    case SYS_CREATE: // bool create (const char *file, unsigned initial_size)
      f->eax = sys_create(f);
      break;
		case SYS_REMOVE: // bool remove (const char *file)
      f->eax = sys_remove(f);
      break;
		case SYS_OPEN:  // int open (const char *file)
      f->eax = sys_open (f);
      break;
		case SYS_FILESIZE:  // int filesize (int fd)
      f->eax = sys_filesize (f);
      break;
		case SYS_READ: // int read (int fd, void *buffer, unsigned size)
      f->eax = sys_read (f);
      break;
		case SYS_WAIT:  // int wait (pid_t pid)
      f->eax = sys_wait (f);
      break;
		case SYS_WRITE: // int write (int fd, const void *buffer, unsigned size)
      f->eax = sys_write (f);
      break;
		case SYS_SEEK: // void seek (int fd, unsigned position)
      sys_seek (f);
      break;
		case SYS_TELL: // unsigned tell (int fd)
      f->eax = sys_tell (f);
      break;
		case SYS_CLOSE: // void close (int fd)
      sys_close (f);
      break; 
    case SYS_CHDIR: // bool chdir (const char *dir)
      f->eax = sys_chdir (f);
      break;
    case SYS_MKDIR: // bool mkdir (const char *dir)
      f->eax = sys_mkdir (f);
      break;
    case SYS_READDIR: // bool readdir (int fd, char *name)
      f->eax = sys_readdir (f);
      break;
    case SYS_ISDIR: // bool isdir (int fd)
      f->eax = sys_isdir (f);
      break;
    case SYS_INUMBER:// int inumber (int fd)
      f->eax = sys_inumber (f);
      break;
		default:
			break;
	}
}

static 
void close_fd (int fd) 
{
	struct file_descriptor* fd_ = check_fd (fd);

  file_close(fd_->file_ptr);
  dir_close (fd_->dir);
  
	list_remove (&fd_->elem);
	free(fd_);
}



static
void* syscall_read_stack (struct intr_frame *f, int locale) 
{
	void* vaddr = f->esp + locale * 4;
  check_pointer((void *) vaddr);

	return vaddr;
}
/* Checks the pointer to see if it's:
 *    - not NULL
 *    - below PHYS_BASE
 *    - in an active page
 *
 *    Exits with -1 on failure of any of these. */
static
void check_pointer(void *p)
{
  if (p == NULL)
  {
    syscall_exit(-1);
  }
  if (p >= PHYS_BASE) 
  { 
    syscall_exit(-1);
  }
  if (pagedir_get_page(thread_current ()->pagedir, p) == NULL)
  {
    syscall_exit(-1);
  }
}

/* If FD is not found, terminate. Otherwise return FD struct */
static
struct file_descriptor* check_fd (int fd)
{
	struct list_elem *e;
	bool found = false;
	struct file_descriptor *fd_ = NULL;
	for (e = list_begin (&thread_current ()->fd_list); 
		e != list_end (&thread_current ()->fd_list);
		e = list_next(e))
		{
			fd_ = list_entry (e, struct file_descriptor, elem);
			if (fd_->fd == fd)
				{
					found = true;
					break;
				}
		}
	if (!found) {
		syscall_exit(-1); }
	return fd_;
}


void syscall_exit (int status)
{
	printf("%s: exit(%d)\n", thread_current ()->name, status);

	/* Close all file descriptors on exit */
	while (!list_empty (&thread_current ()->fd_list))
		{
			struct file_descriptor *fd_ = list_entry (
					list_pop_front(&thread_current ()->fd_list), 
					struct file_descriptor, elem);
      file_close(fd_->file_ptr);
      free (fd_);
		}
 
  lock_acquire(&exiting_lock);
  /* Mark all children as abandoned
   * and reap all un-reaped children*/
  while (!list_empty (&thread_current ()->exit_list))
  {
    struct exit *ex = list_entry(
        list_pop_front(&thread_current ()->exit_list),
        struct exit, elem);

    ex->abandoned = true;
    
    /* If child already exited, reap. */
    if (ex->sema.value > 0)
    {
      free(ex);
    }

  }

  
  /* Mark status, notify parent */
  struct exit *ex = thread_current()->exit;

  ex->status = status;
  sema_up(&ex->sema);
  
  if (ex->abandoned && ex)
  {
    free(ex);
  }

  lock_release(&exiting_lock);

	thread_exit ();
}



/* --------- SYS_* METHODS -----------*/

int sys_exec (struct intr_frame *f)
{
  const char *cmd_line = *(char **) syscall_read_stack(f, 1);
  check_pointer((void *) cmd_line);

  tid_t tid = process_execute (cmd_line);
  return tid;
}

bool sys_create (struct intr_frame *f)
{
  const char *file = *(char **) syscall_read_stack(f, 1);
  check_pointer((void *) file);
  unsigned initial_size = *(unsigned *) syscall_read_stack(f, 2);
  
  lock_acquire (&file_lock);
  bool ans = filesys_create (file, initial_size);	
  lock_release (&file_lock);

  return ans;
}



bool sys_remove (struct intr_frame *f)
{
  //TODO Update so it can remove empty directories
  const char *file = *(char **) syscall_read_stack(f, 1);
  check_pointer((void *) file);


  lock_acquire (&file_lock);
  bool ans = filesys_remove (file);
  lock_release (&file_lock);

  return ans;
}



int sys_open (struct intr_frame *f)
{
  const char *file = *(char **) syscall_read_stack(f, 1);
  check_pointer((void *) file);

  /* Return failure if file does not exist */
  lock_acquire (&file_lock);
  struct file *temp_ptr = filesys_open(file);
  lock_release (&file_lock);

  if (temp_ptr == NULL)
  {
      return -1;
  }


  /* Find the lowest available file descriptor  */
  struct list_elem *e;
  int lowest = 1;
  for (e = list_begin (&thread_current()->fd_list); 
      e != list_end (&thread_current()->fd_list); e = list_next (e))
    {
      struct file_descriptor *temp_fd = list_entry (e, 
          struct file_descriptor, elem);
      
      if (temp_fd->fd > lowest + 1)
          break;
      lowest = temp_fd->fd;
    }

  struct inode *inode = file_get_inode (temp_ptr);

  /* Set up a new file descriptor struct */
  struct file_descriptor *new_fd = malloc(sizeof(struct file_descriptor));
  new_fd->is_dir = inode_is_dir(inode) ? true : false;
  new_fd->dir = new_fd->is_dir ? dir_open(inode_reopen(inode)) : NULL;
  new_fd->fd = lowest + 1;
  new_fd->file_ptr = temp_ptr;
 

  list_insert(e,  &new_fd->elem);

  /* Return the valid file descriptor */
  return lowest + 1;
}


int sys_filesize (struct intr_frame *f)
{
  int fd = *(int *) syscall_read_stack(f, 1);

  lock_acquire (&file_lock);
  int ans = file_length(check_fd (fd)->file_ptr);
  lock_release (&file_lock);

  return ans;
}


int sys_read (struct intr_frame *f)
{
  int fd = *(int *) syscall_read_stack(f, 1);
  void* buffer = *(void**)syscall_read_stack(f, 2);
  check_pointer(buffer);
  unsigned size = *(unsigned *) syscall_read_stack(f, 3);

  if (fd == 0)
    {
      int bytes_read = 0;
      uint8_t key = 0;
      while (key != '\r' && size > 0)
      {
        key = input_getc();
        memcpy(buffer, &key, 1);
        buffer++;
        size--;
        bytes_read++;
      }

      return bytes_read;
    }
  else // Read from file
    {
      struct file_descriptor *fd_ = check_fd (fd);

      return file_read(fd_->file_ptr, buffer, size);
    }
}

int sys_wait (struct intr_frame *f)
{
        tid_t pid = *(tid_t *)syscall_read_stack (f, 1);
        return process_wait(pid);
}

int sys_write (struct intr_frame *f)
{
  lock_acquire (&file_lock);
  int fd = *(int *) syscall_read_stack(f, 1);
  void *buffer = *(void **) syscall_read_stack(f, 2);
  check_pointer(buffer);
  unsigned size = *(unsigned *)syscall_read_stack(f, 3);
  
  // Amount written to file
  int ans = 0;

  // Print to STDOUT
  if (fd == 1) 
  {
    if (size > 400) 
    {
      // If size too big, break buffer into chunks
      while (size > 400)
      {
        putbuf(buffer, 400);
        buffer += 400;
        size -= 400;
        ans += 400;
      }
    }
    putbuf(buffer, size);
    ans += size;
  }
  else 
  {
     struct file_descriptor *file_d = check_fd(fd);
     if (file_d->is_dir)
     {
       ans = -1;
     }
     else 
     {
       struct file *file = file_d->file_ptr;
       ans = file_write (file, buffer, size);
     }
  }
  lock_release (&file_lock);

  return ans;
}

void sys_seek (struct intr_frame *f)
{
  int fd = *(int *) syscall_read_stack(f, 1);
  unsigned pos = *(unsigned *) syscall_read_stack(f, 2);

  struct file_descriptor *fd_ = check_fd (fd);

  lock_acquire (&file_lock);
  file_seek(fd_->file_ptr, pos);
  lock_release (&file_lock);
}


unsigned sys_tell (struct intr_frame *f)
{
  int fd = *(int *) syscall_read_stack(f, 1);
  struct file_descriptor *fd_ = check_fd (fd);
  lock_acquire (&file_lock);
  unsigned ans = file_tell(fd_->file_ptr);
  lock_release (&file_lock);

  return ans;
}


void sys_close (struct intr_frame *f)
{
  int fd = *(int *) syscall_read_stack(f, 1);
  close_fd (fd);
}


/* Changes the current working directory to dir 
 * Returns true on success.*/
bool sys_chdir (struct intr_frame *f)
{
  const char *dir = *(char **) syscall_read_stack(f, 1);
  check_pointer ((void *) dir);
  
  lock_acquire(&file_lock);
  bool ans = filesys_chdir(dir);
  lock_release(&file_lock);

  return ans;
}


/* Creates the directory dir. Returns true on success.
 * Fails if dir already exists, or any directory name
 * in the path does not exist */
bool sys_mkdir (struct intr_frame *f)
{
  const char *dir = *(char **) syscall_read_stack(f, 1);
  check_pointer ((void *) dir);

  lock_acquire(&file_lock);
  bool ans = filesys_mkdir(dir);
  lock_release(&file_lock);

  return ans;
}


/* Reads a directory from fd. If successful, stores
 * the null terminated file name in name, which must
 * have room for READDIR_MAX_LEN + 1 bytes, and returns
 * true. If no entries left in the directory, return false. 
 *
 * If directory changes while open, no worries. 
 *
 * Do not return "." or ".."
 *
 * READDIR_MAX_LEN defined in lib/user/syscall.h. Will
 * probably have to change this */
bool sys_readdir (struct intr_frame *f)
{
  int fd_ = *(int *) syscall_read_stack(f, 1); 
  char *name = *(char **) syscall_read_stack(f, 2);
  check_pointer ((void *) name);

  struct file_descriptor *fd = check_fd(fd_);

  return fd->is_dir ? dir_readdir (fd->dir, name) : false;
}



bool sys_isdir (struct intr_frame *f)
{
  int fd_ = *(int *) syscall_read_stack(f, 1); 
  struct file_descriptor *fd = check_fd(fd_);
  return fd->is_dir;
}



/* Returns the inodenumber of the inode associated with fd,
 * which may represent a file or directory */
int sys_inumber (struct intr_frame *f)
{
  int fd = *(int *) syscall_read_stack(f, 1); 
  struct file *file = check_fd(fd)->file_ptr;
  return file_inumber(file);
}



