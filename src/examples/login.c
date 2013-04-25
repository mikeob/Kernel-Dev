#include <stdio.h>
#include <syscall.h>
#include <string.h>
#include <stdlib.h>
#include "verify.h"
#include "cmdline.h"
#include "hash.h"

int
main (void) 
{
	char username [20];
  char userpasswd [20];
  int i;
  bool verified = false;
	char uid [6];
	char gid [6];

	/* Get first user login attempt. */
	printf ("Username: ");
  read_line (username, sizeof username, false);
  printf ("Password: ");
  read_line (userpasswd, sizeof userpasswd, true);
	
  /* Attempt to open passwd and shadow. */
  int passwd_fd = open ("etc/passwd");
	int shadow_fd = open ("etc/shadow");

	/* Create passwd if it does not exist and open it adding
   * root to passwd. */
  if (passwd_fd == -1)
	{
		create ("etc/passwd", 10000);
		passwd_fd = open ("etc/passwd");
		write (passwd_fd, "root:x:0:0\n", 11);
		write (passwd_fd, "kevin:x:1:1\n", 12);
		write (passwd_fd, "ollie:x:2:2\n", 12);
		write (passwd_fd, "mike:x:3:3\n", 11);
	}

  /* Create shadow if it does not exist and open it adding
   * root to shadow. */
  if (shadow_fd == -1)
	{
		create ("etc/shadow", 10000);
		shadow_fd = open ("etc/shadow");
		
		char hashed_passwd [33];
		char root [5] = "root";
		char kevin [6] = "kevin";
		char ollie [6] = "ollie";
		char mike [5] = "mike";
		char colon [1] = ":";
		char new_line [1] = "\n";

		char user_entry [100];

		md5 ((uint8_t *)root, strlen(root), hashed_passwd);
		strlcpy (user_entry, root, strlen (root) + 1);
		strlcat (user_entry, colon, strlen (user_entry) + strlen (colon) + 1);
		strlcat (user_entry, hashed_passwd, strlen (user_entry) + strlen (hashed_passwd) + 1);
		strlcat (user_entry, new_line, strlen (user_entry) + strlen (new_line) + 1);
		write (shadow_fd, user_entry, strlen (user_entry) + 1);

		memset (user_entry, 0, 100);
		md5 ((uint8_t *)kevin, strlen(kevin), hashed_passwd);
		strlcpy (user_entry, kevin, strlen (kevin) + 1);
		strlcat (user_entry, colon, strlen (user_entry) + strlen (colon) + 1);
		strlcat (user_entry, hashed_passwd, strlen (user_entry) + strlen (hashed_passwd) + 1);
		strlcat (user_entry, new_line, strlen (user_entry) + strlen (new_line) + 1);
		write (shadow_fd, user_entry, strlen (user_entry) + 1);
	
		memset (user_entry, 0, 100);
		md5 ((uint8_t *)ollie, strlen(ollie), hashed_passwd);
		strlcpy (user_entry, ollie, strlen (ollie) + 1);
		strlcat (user_entry, colon, strlen (user_entry) + strlen (colon) + 1);
		strlcat (user_entry, hashed_passwd, strlen (user_entry) + strlen (hashed_passwd) + 1);
		strlcat (user_entry, new_line, strlen (user_entry) + strlen (new_line) + 1);
		write (shadow_fd, user_entry, strlen (user_entry) + 1);

		memset (user_entry, 0, 100);
		md5 ((uint8_t *)mike, strlen(mike), hashed_passwd);	
		strlcpy (user_entry, mike, strlen (mike) + 1);
		strlcat (user_entry, colon, strlen (user_entry) + strlen (colon) + 1);
		strlcat (user_entry, hashed_passwd, strlen (user_entry) + strlen (hashed_passwd) + 1);
		strlcat (user_entry, new_line, strlen (user_entry) + strlen (new_line) + 1);
		write (shadow_fd, user_entry, strlen (user_entry) + 1);

	
	}

  /* Attempt first verification. */
	verified = verify (username, userpasswd, uid, gid);

  /* If verification failed, give user nine more tries. */
	if (!verified)
	{
		for (i = 0; i < 9; i++)
		{
			printf("Login failed.\n");
			printf ("Username: ");
 			read_line (username, sizeof username, false);
 			printf ("Password: ");
 			read_line (userpasswd, sizeof userpasswd, true);
			verified = verify (username, userpasswd, uid, gid);
			if (verified)
				break;
		}
	}
	
	/* Close passwd and shadow. */
	close (passwd_fd);
	close (shadow_fd);
  
	/* If verification was successful, run the shell. */
	if (verified)
	{
		setgid (atoi (gid));
		setuid (atoi (uid));
	
		wait (exec ("shell"));
	}
  else
  {
    return EXIT_FAILURE;
  }
	
	return EXIT_SUCCESS;
}

