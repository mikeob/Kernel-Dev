#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <list.h>
#include "threads/thread.h"

void syscall_init (void);
void syscall_exit (int status);

struct file_descriptor
{
	int fd;
	struct file * file_ptr;
	struct list_elem elem;
};

#endif /* userprog/syscall.h */
