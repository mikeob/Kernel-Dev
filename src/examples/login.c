#include <stdio.h>
#include <syscall.h>
#include <string.h>
#include <stdlib.h>
#include "verify.h"

static void read_line (char line[], size_t, bool password);
static bool backspace (char **pos, char line[]);

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
		write (shadow_fd, "root:root\n", 10);
		write (shadow_fd, "kevin:kevin\n", 12);
		write (shadow_fd, "ollie:ollie\n", 12);
		write (shadow_fd, "mike:mike\n", 10);
	}

  /* Attempt first verification. */
	verified = verify (username, userpasswd, passwd_fd, shadow_fd, uid, gid);

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
			verified = verify (username, userpasswd, passwd_fd, shadow_fd, uid, gid);
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
	
	return EXIT_SUCCESS;
}

/* Reads a line of input from the user into LINE, which has room
   for SIZE bytes.  Handles backspace and Ctrl+U in the ways
   expected by Unix users.  On return, LINE will always be
   null-terminated and will not end in a new-line character. */
static void
read_line (char line[], size_t size, bool password) 
{
  char *pos = line;
  for (;;)
    {
      char c;
      read (STDIN_FILENO, &c, 1);

      switch (c) 
        {
        case '\r':
          *pos = '\0';
          putchar ('\n');
          return;

        case '\b':
          backspace (&pos, line);
          break;

        case ('U' - 'A') + 1:       /* Ctrl+U. */
          while (backspace (&pos, line))
            continue;
          break;

        default:
          /* Add character to line. */
          if (pos < line + size - 1) 
            {
							if (password)
								putchar ('*');
							else
              	putchar (c);
              *pos++ = c;
            }
          break;
        }
    }
}

/* If *POS is past the beginning of LINE, backs up one character
   position.  Returns true if successful, false if nothing was
   done. */
static bool
backspace (char **pos, char line[]) 
{
  if (*pos > line)
    {
      /* Back up cursor, overwrite character, back up
         again. */
      printf ("\b \b");
      (*pos)--;
      return true;
    }
  else
    return false;
}
