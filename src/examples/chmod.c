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

  //TODO Implement suid bit access through chmod
  //
  //





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
	int setuid_p = -1;

	if (atoi(argv[1]) > 777)
	{
		user_p = mask % 10;
		mask /= 10;
		setuid_p = mask;
	}

  if (user_p > 7 || group_p > 7 || others_p > 7 || setuid_p % 2 == 1 || atoi(argv[1]) > 7777)
  {
    printf("Invalid permission map.\n");
    return EXIT_FAILURE;
  }

	//char file [100] = "";
	//strlcpy (file, argv[2], 100);

  chmod (argv[2], atoi(argv[1]));

	return EXIT_SUCCESS;
}
