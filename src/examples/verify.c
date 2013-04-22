#include <stdio.h>
#include <syscall.h>
#include <string.h>
#include <stdlib.h>
#include "verify.h"

bool
verify (char *username, char *userpasswd, int passwd_fd, int shadow_fd, char *uid, char *gid)
{
	char temp_string [20];
	char *insert;
	char c;
	int i, fd = passwd_fd;

  /* Start at beginning of both files. */
	seek (passwd_fd, 0);
	seek (shadow_fd, 0);

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

    /* If username entered matches a username in passwd,
     * switch to shadow file and find username in it. */
		if (!strcmp (username, temp_string) && fd == passwd_fd)
		{
				fd = shadow_fd;
		}

    /* If username entered and found in passwd matches
     * a username in shadow, verify the password matches. */
		else if (!strcmp (username, temp_string))
		{
			insert = temp_string;
			memset (temp_string, 0, 20);

			/* Grab the password in shadow. */
			for (i = 0; i < 20; i++)
			{
				read (fd, &c, 1);
				if (c == '\n')
					break;
    		memcpy (insert, &c, 1);
				insert++;
			}

			/* If the password matches then return true for
       * verification. */
			if (!strcmp (userpasswd, temp_string)) 
			{
				char user[100];
				memset(user, 0, 100);
				seek(passwd_fd, 0);

				for (;;)
				{
					memset(temp_string, 0, 20);
					insert = temp_string;
					for (i = 0; i < 20; i++)
						{						
							read (passwd_fd, &c, 1);
							if (c == ':')
								break;
							memcpy (insert, &c, 1);
							insert++;
						}
					if (!strcmp (username, temp_string))
						break;
					else
						{
							while (c != '\n' && c != '\0') {
								read (passwd_fd, &c, 1);
							}
						}
				}
				
				insert = user;
				for (i = 0; i < 100; i++)
					{
						read(passwd_fd, &c, 1);
						if (c == '\n')
							break;
						memcpy (insert, &c, 1);
						insert++;
					}	

				char* ptr = "";
				char** saveptr = &ptr;

				/* First call to strtok_r returns 'x', because password is in shadow */
				strtok_r (user, ":", saveptr);

				if (uid != NULL)	
					strlcpy(uid, strtok_r (NULL, ":", saveptr), 6);
				
				if (gid != NULL)				
					strlcpy(gid, strtok_r (NULL, ":", saveptr), 6);
	
				return true;
			}
	
			/* Password did not match. */
			return false;
		}

		/* If username entered does not match the username we 
     * just grabbed from the file, grab next username in file. */
		else
		{
			while (c != '\n' && c != '\0')
				read (fd, &c, 1);

			if (read (fd, &c, 1) == 0 || c == '\0')
				break;
		
			seek (fd, tell (fd) - 1);
		}
	}

  /* Username was not found. */
	return false;
}
