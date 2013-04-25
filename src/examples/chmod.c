#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <string.h>


/* Chmod program. Passes a permissions integer of the form <xyz>
 * where 0 <= x, y, z <= 7, followed by a file.
 *
 * For example: 
 *
 * chmod 740 ./hello_world.py
 *
 * */

int 
main (int argc, char *argv[])
{
  /*   PREVIOUS VERSION
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
  */

  if (argc != 3)
  {
    printf("Invalid number of arguments. Must pass 3 arguments.\n");
    return EXIT_FAILURE;
  }

  /* Extract the permission mapping */
  int mask = atoi(argv[1]);
  int others_p = mask % 10;
  mask /= 10;
  int group_p = mask % 10;
  mask /= 10;
  int user_p = mask;

  if (user_p > 7 || group_p > 7 || others_p > 7)
  {
    printf("Invalid permission map. Please enter 
        3 digits between 0-7 inclusive\n");
    return EXIT_FAILURE;
  }

  chmod (argv[2], user_p, others_p, group_p);

	return EXIT_SUCCESS;
}
