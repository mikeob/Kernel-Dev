#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <list.h>
#include "threads/thread.h"
#include "filesys/directory.h"

void syscall_init (void);
void syscall_exit (int status);

struct file_descriptor
{
	int fd;
  bool is_dir; // Indicates whether or not this points to a directory
	struct file * file_ptr;
  struct dir * dir;
	struct list_elem elem;
};

#endif /* userprog/syscall.h */
