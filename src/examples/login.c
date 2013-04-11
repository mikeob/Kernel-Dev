#include <stdio.h>
#include <syscall.h>
#include <string.h>

static void read_line (char line[], size_t, bool password);
static bool backspace (char **pos, char line[]);
static bool verify (char *username, char *userpasswd, int passwd_fd, int shadow_fd);

int
main (void) 
{
	char username [20];
  char userpasswd [20];
  int i;
  bool verified = false;

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
	}

  /* Create shadow if it does not exist and open it adding
   * root to shadow. */
  if (shadow_fd == -1)
	{
		create ("etc/shadow", 10000);
		shadow_fd = open ("etc/shadow"); 
		write (shadow_fd, "root:root\n", 10);
	}

  /* Attempt first verification. */
	verified = verify (username, userpasswd, passwd_fd, shadow_fd);

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
			verified = verify (username, userpasswd, passwd_fd, shadow_fd);
			if (verified)
				break;
		}
	}

  /* If verification was successful, run the shell. */
	if (verified)
		wait (exec ("shell"));

	return EXIT_SUCCESS;
}

static bool
verify (char *username, char *userpasswd, int passwd_fd, int shadow_fd)
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
				return true;
	
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
