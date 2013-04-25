/* User program to run a command as root
 * Must:
 * 	- have setuid bit set
 * 	- be owned by root
 */

#include <string.h>
#include <stdio.h>
#include <syscall.h>

#define MAX_LENGTH 100

int main (int argc, char* argv[])
{
	if (argc == 1)
		{
			// Only passed sudo, do nothing
			return 0;
		}
	// Implicit else
	
	// Check that user is a sudoer

	// Check password of user
	
	// Figure out length of arguments
	int i, length = 0;
	for (i = 1; i < argc; i++)
		{
			length += strlen(argv[i]);
			// Add a space between arguments.
			if (i != argc - 1)
				length += 1;
		}

	printf("%d\n", length);
	
	// Impose some reasonable cmd limit
	if (length > MAX_LENGTH)
		return -1;

	char addr[MAX_LENGTH] = "\0";
	char* cmdline = &addr[0];
	for (i = 1; i < argc; i++)
		{
			strlcat(cmdline, argv[i], MAX_LENGTH); 
			if (i != argc - 1)
				strlcat(cmdline, " ", MAX_LENGTH);
		}

	exec (cmdline);
	return 0;
}
