#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include "cmdline.h"

/* Reads a line of input from the user into LINE, which has room
   for SIZE bytes.  Handles backspace and Ctrl+U in the ways
   expected by Unix users.  On return, LINE will always be
   null-terminated and will not end in a new-line character. */
void
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
        case 10: // \r and \n
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
bool
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
