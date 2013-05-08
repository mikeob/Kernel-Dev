/* touch.c

   Creates files specified on command line */

#include <stdio.h>
#include <syscall.h>

#define FILE_SIZE 1024

int
main (int argc, char *argv[]) 
{
  bool success = true;
  int i;
  
  for (i = 1; i < argc; i++) 
    {
      int created = create (argv[i], FILE_SIZE);
      if (!created) 
        {
          printf ("%s: create failed\n", argv[i]);
          success = false;
        }
    }
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
