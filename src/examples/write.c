/* write.c

   Writes text to a particular file */

#include <stdio.h>
#include <syscall.h>
#include <string.h>

int
main (int argc, char *argv[]) 
{

	char* programname = argv[0];
	if (argc != 3) 
		{
			printf("%s: invalid argc, exiting\n", programname);
			return EXIT_FAILURE;
		}

	char* filename = argv[1];
	char* text = argv[2];

	int fd = open (filename);
	if (fd < 0) 
  	{
    	printf ("%s: open failed\n", programname);
      return EXIT_FAILURE;
    }

	int text_len = strlen(text);
	int written = write (fd, text, text_len);
	if (written < 0)
		{
			printf("%s: %s: Permission denied.\n", programname, filename);
			close(fd);
			return EXIT_FAILURE;
		}
	close (fd);
  return EXIT_SUCCESS;
}
