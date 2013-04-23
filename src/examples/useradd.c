#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

int main (int argc, const char *argv[])
{
	char username [21];
	int previous_id = 0;
	int current_id = 0;
	int new_id = 0;
	char temp_string [20];
	char *insert;
	char c;
	int i;

	if (argc <= 1)
	{
		printf ("No username entered.\n");
		return 0;
	}

	strlcpy (username, argv[1], sizeof username);

	int fd = open ("etc/passwd");

	/* Loop through files. */
	for (;;)
	{
		insert = temp_string;
		memset (temp_string, 0, 20);
	
    /* Read username. */
		for (i = 0; i < 20; i++)
		{
			read (fd, &c, 1);
			if (c == ':')
				break;
   	 	memcpy (insert, &c, 1);
			insert++;
		}

		if (!strcmp (username, temp_string))
		{
			printf ("User already exists.\n");
			return 0;
		}

		seek (fd, tell(fd) + 2);

		memset (temp_string, 0, 20);
		insert = temp_string;

		/* Read id. */
		for (i = 0; i < 5; i++)
		{
			read (fd, &c, 1);
			if (c == ':')
				break;
			memcpy (insert, &c, 1);
			insert++;
		}

    if (new_id == 0)
		{
			current_id = atoi (temp_string);

			if (current_id > previous_id + 1)
				new_id = previous_id + 1;

			previous_id = atoi (temp_string);
		}

		while (c != '\n' && c != '\0')
				read (fd, &c, 1);

			if (read (fd, &c, 1) == 0 || c == '\0')
				break;
		
			seek (fd, tell (fd) - 1);
	}

	if (new_id == 0)
	{
		new_id = previous_id + 1;
	}

	char user_entry [100];
	char colon [1] = ":";
	char passwd [1] = "0";
	char new_line [1] = "\n";
	char id [6];
 	snprintf (id, 6, "%d", new_id);
	
	memcpy (user_entry, username, sizeof username);
	strlcat (user_entry, colon, strlen (user_entry) + 2);
  strlcat (user_entry, passwd, strlen (user_entry) + 2);
	strlcat (user_entry, colon, strlen (user_entry) + 2);
	strlcat (user_entry, id, strlen (user_entry) + strlen (id) + 1);
	strlcat (user_entry, colon, strlen (user_entry) + 2);
	strlcat (user_entry, id, strlen (user_entry) + strlen (id) + 1);
	strlcat (user_entry, new_line, strlen (user_entry) + 2);

	write (fd, user_entry, strlen (user_entry)); 

	close (fd);

	return 0;
}
