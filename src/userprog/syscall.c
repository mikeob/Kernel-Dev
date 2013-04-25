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
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);
static void* syscall_read_stack (struct intr_frame *f, int locale);
void syscall_exit (int status);
static struct lock file_lock;
static void check_pointer(void *p);
static struct file_descriptor* check_fd (int fd);
static void close_fd (int fd);
static struct lock exiting_lock;



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
		case SYS_HALT:
      { 
				shutdown_power_off();
			  break;
      }
		case SYS_EXIT:
			{
				int status = *(int *)syscall_read_stack(f, 1);
				syscall_exit (status);
				break;
			}
		case SYS_EXEC: 
      {
        //TODO CHECK PERMISSIONS
			  const char *cmd_line = *(char **) syscall_read_stack(f, 1);
        check_pointer((void *) cmd_line);
			
        tid_t tid = process_execute (cmd_line);
			  f->eax = tid;
				break;
      }
    case SYS_CREATE:
      {
        //TODO SET FILE PERMISSIONS
        const char *file = *(char **) syscall_read_stack(f, 1);
        check_pointer((void *) file);

        unsigned initial_size = *(unsigned *) syscall_read_stack(f, 2);
				
				lock_acquire (&file_lock);
				f->eax = filesys_create (file, initial_size);	
				lock_release (&file_lock);
				 
			  break;
      }
		case SYS_REMOVE: 
      {
        //TODO Update so it can remove empty directories
        //TODO CHECK PERMISSIONS
        const char *file = *(char **) syscall_read_stack(f, 1);
        check_pointer((void *) file);
			  lock_acquire (&file_lock);
				f->eax = filesys_remove (file);
				lock_release (&file_lock);
        break;
      }
		case SYS_OPEN:
      {
        //TODO CHECK PERMISSIONS
        // Can we open a file with no permissions? I suppose we can...
        // Though, nothing can be done with the file..
	
        const char *file = *(char **) syscall_read_stack(f, 1);
        check_pointer((void *) file);

				/* Return failure if file does not exist */
				lock_acquire (&file_lock);
				struct file *temp_ptr = filesys_open(file);
				lock_release (&file_lock);
				if (temp_ptr == NULL)
					{
						f->eax = -1;
						break;
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
			
				/* Set up a new file descriptor struct */
				struct file_descriptor *new_fd = malloc(sizeof(struct file_descriptor));
        new_fd->is_dir = file_is_dir(temp_ptr);
        new_fd->fd = lowest + 1;
				new_fd->file_ptr = temp_ptr;
				list_insert(e,  &new_fd->elem);

				/* Return the valid file descriptor */
				f->eax = lowest + 1;
				break;
			}
		case SYS_FILESIZE:
      {
        int fd = *(int *) syscall_read_stack(f, 1);
				lock_acquire (&file_lock);
				f->eax = file_length(check_fd (fd)->file_ptr);
				lock_release (&file_lock);
			  break;
      }
		case SYS_READ: 
      {
        //TODO CHECK FILE PRIVILEGES
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
						f->eax = bytes_read;
					}
				else
					{
						struct file_descriptor *fd_ = check_fd (fd);
						f->eax = file_read(fd_->file_ptr, buffer, size);
					}
			  break;
      }
		case SYS_WAIT:
      {
        tid_t pid = *(tid_t *)syscall_read_stack (f, 1);
        f->eax = process_wait(pid);
        break;
      }
		case SYS_WRITE:
			{
        //TODO CHECK FILE PRIVILEGES!
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

        f->eax = ans;
				break;
			}
		case SYS_SEEK: 
      {
        int fd = *(int *) syscall_read_stack(f, 1);
        unsigned pos = *(unsigned *) syscall_read_stack(f, 2);
				struct file_descriptor *fd_ = check_fd (fd);
				lock_acquire (&file_lock);
				file_seek(fd_->file_ptr, pos);
				lock_release (&file_lock);
			  break;
      }
		case SYS_TELL:
      {
        int fd = *(int *) syscall_read_stack(f, 1);
				struct file_descriptor *fd_ = check_fd (fd);
				lock_acquire (&file_lock);
				f->eax = file_tell(fd_->file_ptr);
				lock_release (&file_lock);
			  break;
      }
		case SYS_CLOSE: 
      {
        int fd = *(int *) syscall_read_stack(f, 1);
				close_fd (fd);
				break;
      }
		case SYS_SETUID:
			{
				/* sets the effective user ID of the calling process. 
				 * If the effective UID of the caller is root, 
				 * the real UID and saved set-user-ID are also set. */
				int id = *(int *) syscall_read_stack(f, 1);
				if (thread_current ()->euid == 0) 
					{
						thread_current ()->euid = id;
						thread_current ()->suid = id;
						thread_current ()->ruid = id;
					}
				else
					{
						/* Do not let unprivileged users gain privileged access */
						if (id == 0)
							{
								f->eax = -1;
								break;
							}
						else
							thread_current ()->euid = id;
					}
				break;
			}
		case SYS_SETEUID:
			{
				/* sets the effective user ID of the calling process. 
				 * Unprivileged user processes may only set the effective user ID to 
				 * the real user ID, the effective user ID or the saved set-user-ID. */
				int id = *(int *) syscall_read_stack(f, 1);
				if (thread_current ()->ruid > 0 || thread_current ()->euid > 0) 
					{
					/* Unprivileged user */
						if (id != thread_current()->ruid || id != thread_current ()->euid 
								|| id != thread_current()->suid)
							{
								thread_current ()->euid = id;
							}
						else 
							{
								f->eax = -1;
								break;
							}
					}
				else 
					{
						thread_current ()->euid = id;
					}
				f->eax = 0;
				break;
			}
		case SYS_SETGID:
			{
				/* sets the effective group ID of the calling process. 
				 * If the caller is the superuser, the real GID and saved set-group-ID 
				 * are also set. */
				int id = *(int *) syscall_read_stack(f, 1);
				if (thread_current ()->euid == 0) 
					{
						thread_current ()->egid = id;
						thread_current ()->sgid = id;
						thread_current ()->rgid = id;
					}
				else
					{
						/* Do not let unprivileged users gain privileged access */
						if (id == 0)
							{
								f->eax = -1;
								break;
							}
						else
							thread_current ()->egid = id;
					}
				f->eax = 0;
				break;
			}
		case SYS_SETEGID:
			{
				/* sets the effective group ID of the calling process. 
				 * Unprivileged user processes may only set the effective group ID to 
				 * the real group ID, the effective group ID or the saved set-group-ID. */
				int id = *(int *) syscall_read_stack(f, 1);
				if (thread_current ()->ruid > 0 || thread_current ()->euid > 0) 
					{
					/* Unprivileged user */
						if (id != thread_current()->rgid || id != thread_current ()->egid 
								|| id != thread_current()->sgid)
							{
								thread_current ()->egid = id;
							}
						else
							{
								f->eax = -1;
								break;
							}
					}
				else 
					{
						thread_current ()->egid = id;
					}
				f->eax = 0;
				break;
			}
		case SYS_GETUID:
			{
				f->eax = thread_current ()->ruid;
				break;
			}
		case SYS_GETEUID:
			{
				f->eax = thread_current ()->euid;
				break;
			}
		case SYS_GETGID:
			{
				f->eax = thread_current ()->rgid;
				break;
			}
		case SYS_GETEGID:
			{
				f->eax = thread_current ()->egid;
				break;
			}
		case SYS_CHMOD:
			{
        //TODO INCOMPLETE
				f->eax = 0;
				break;
			}
      /* Changes the current working directory to dir 
       * Returns true on success.*/
    case SYS_CHDIR: // bool chdir (const char *dir)
      {
        const char *dir = *(char **) syscall_read_stack(f, 1);
        check_pointer ((void *) dir);
        
        lock_acquire(&file_lock);
        f->eax = filesys_chdir(dir);
        lock_release(&file_lock);
        break;
      }


      /* Creates the directory dir. Returns true on success.
       * Fails if dir already exists, or any directory name
       * in the path does not exist */
    case SYS_MKDIR: // bool mkdir (const char *dir)
      {
        const char *dir = *(char **) syscall_read_stack(f, 1);
        check_pointer ((void *) dir);
        lock_acquire(&file_lock);
        f->eax = filesys_mkdir(dir);
        lock_release(&file_lock);
        break;
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
    case SYS_READDIR: // bool readdir (int fd, char *name)
      {
        //TODO INCOMPLETE
        int fd_ = *(int *) syscall_read_stack(f, 1); 
        const char *name = *(char **) syscall_read_stack(f, 2);
        check_pointer ((void *) name);

        struct file_descriptor *fd = check_fd(fd_);
        if (!fd->is_dir)
        {
          f->eax = false;
          break;
        }

        break;
      }
    case SYS_ISDIR: // bool isdir (int fd)
      {
        int fd_ = *(int *) syscall_read_stack(f, 1); 
        struct file_descriptor *fd = check_fd(fd_);
        f->eax = fd->is_dir;
        break;
      }
      /* Returns the inodenumber of the inode associated with fd,
       * which may represent a file or directory */
    case SYS_INUMBER:// int inumber (int fd)
      {
        int fd = *(int *) syscall_read_stack(f, 1); 
        struct file_descriptor *file_d = check_fd(fd);
        f->eax = file_get_inumber(file_d->file_ptr);
        break;
      }
		default:
			break;
	}
}

static 
void close_fd (int fd) 
{
	struct file_descriptor* fd_ = check_fd (fd);

  file_close(fd_->file_ptr);
  
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
    //printf("Freeing %p\n", ex);
    free(ex);
  }

  lock_release(&exiting_lock);

	thread_exit ();
}
