#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <string.h>

int 
main (int argc, char *argv[])
{
	int mode = -1;
  char * file;

	if (argc <= 2)
	{
		printf ("Too few arguments.\n");
		return 0;
	}

	mode = atoi (argv[1]);

	char mode_str [5];
	char next_byte [2];
	char *byte_ptr = mode_str;
	
	strlcpy (mode_str, argv[1], strlen (argv[1]) + 1);

	memcpy (next_byte, mode_str, 1);
	int temp_num = atoi (next_byte);

	if ((strlen (mode_str) == 4 && (temp_num != 2 && temp_num != 4)) || strlen (mode_str) < 3 || strlen (mode_str) > 4)
	{
		printf ("Invalid mode.\n");
		return 0;
	}

	for (;;)
	{
		if (*byte_ptr == '\0')
			break;
 
		if (temp_num > 7)
		{
			printf ("Invalid mode.\n");
			return 0;
		}

		memcpy (next_byte, ++byte_ptr, 1);
		temp_num = atoi (next_byte);
	}

	file = argv [2];

	if (file == NULL)
		printf ("Invalid file.\n");

	chmod (file, mode);

	return 0;
}
