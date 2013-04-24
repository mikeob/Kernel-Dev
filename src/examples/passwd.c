#include <syscall.h>
#include <stdio.h>
#include <string.h>
#include "cmdline.h"
#include "hash.h"

int main (void)
{
	char uid [6]; 
	snprintf (uid, 6, "%d", getuid());	
	char temp_string [20];
	char username [20];
	char *insert;
	char c;
	int i;
	int total_chars = 0;
	bool passwd_already = false;
	bool user_found = false;

	int fd = open ("etc/passwd");

	char username_root [20];
  char userpasswd [20];
	char passwd_verify [20];

	/* If root, get username. */
	if (getuid() == 0)
	{
		printf ("Username: ");
		read_line (username_root, sizeof username_root, false);

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

			/* Make sure user is in passwd. */
			if (!strcmp(username_root, temp_string))
			{
				user_found = true;
				break;
			}

			while (c != '\n' && c != '\0')
				read (fd, &c, 1);

			if (read (fd, &c, 1) == 0 || c == '\0')
				break;
		
			seek (fd, tell (fd) - 1);
		}

		/* Exit if the user doesn't exist. */
		if (!user_found)
		{
			printf ("User not found.\n");
			return 0;
		}	

		/* Skip to id. */
		seek (fd, tell(fd) + 2);

		insert = temp_string;
		memset (temp_string, 0, 20);

		/* Read id. */
		for (i = 0; i < 5; i++)
		{
			read (fd, &c, 1);
			if (c == ':')
				break;
			memcpy (insert, &c, 1);
			insert++;
		}
		memcpy (uid, temp_string, sizeof temp_string);
	}


	seek (fd, 0);

	/* Request new password. */
	printf ("New password: ");
	read_line (userpasswd, sizeof userpasswd, true);
	printf ("Re-enter password: ");
	read_line (passwd_verify, sizeof passwd_verify, true);

	/* If passwords don't match. */
	while (strcmp (userpasswd, passwd_verify))
	{
		printf ("Passwords do not match. Please try again.\n");
		printf ("New password: ");
		read_line (userpasswd, sizeof userpasswd, true);
		printf ("Re-enter password: ");
		read_line (passwd_verify, sizeof passwd_verify, true);
	}
	
	/* Loop through files. */
	for (;;)
	{
		insert = temp_string;
		memset (temp_string, 0, 20);
		total_chars = 0;

		/* Read username. */
		for (i = 0; i < 20; i++)
		{
			read (fd, &c, 1);
			if (c == ':')
				break;
   	 	memcpy (insert, &c, 1);
			insert++;
		}
	
		memcpy (username, temp_string, sizeof temp_string); 

		seek (fd, tell(fd) + 2);

		insert = temp_string;
		memset (temp_string, 0, 20);

		/* Read id. */
		for (i = 0; i < 5; i++)
		{
			read (fd, &c, 1);
			total_chars++;
			if (c == ':')
				break;
			memcpy (insert, &c, 1);
			insert++;
		}

		/* If user if found. */
		if (!strcmp (uid, temp_string))
			break;

		while (c != '\n' && c != '\0')
				read (fd, &c, 1);

		if (read (fd, &c, 1) == 0 || c == '\0')
			break;
		
		seek (fd, tell (fd) - 1);
	}

	/* Go back to check for x. */
	seek (fd, tell(fd) - total_chars - 2);

	read (fd, &c, 1);

	/* Make sure user has password entry in shadow. */
	if (c == 'x')
		passwd_already = true;
	else
	{
		char x [1] = "x";
		seek (fd, tell (fd) - 1);
		write (fd, x, 1);
	}

	close (fd);
	fd = open ("etc/shadow");
	
	/* Hash and write new password. */
	if (passwd_already)
	{
		/* Hash the new password. */
		char hashed_passwd [33];
		md5 ((uint8_t*)userpasswd, strlen(userpasswd), hashed_passwd);
		
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

			/* If the user was found. */
			if (!strcmp (username, temp_string))
				break;
	
			while (c != '\n' && c != '\0')
					read (fd, &c, 1);

			if (read (fd, &c, 1) == 0 || c == '\0')
				break;
		
			seek (fd, tell (fd) - 1);
		}

		/* Write the hashed password. */
		write (fd, hashed_passwd, 32);
		printf ("Password changed.\n");
	}

	return 0;
}
