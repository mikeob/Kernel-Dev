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

	/* Exit if etc/passwd is not present. */
  if (passwd_fd == -1)
	{
		printf("File etc/passwd not found.\n");
		return -1;
	}

  /* Exit if etc/shadow is not present. */
  if (shadow_fd == -1)
	{
		printf("File etc/shadow not found.\n");
		return -1;
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

