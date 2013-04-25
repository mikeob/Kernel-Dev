/* User program to run a command as root
 * Must:
 * 	- have setuid bit set
 * 	- be owned by root
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include "cmdline.h"
#include "hash.h"

#define MAX_LENGTH 100

int main (int argc, char* argv[])
{
	if (argc == 1)
		{
			// Only passed sudo, do nothing
			return -1;
		}

	char* passwd_file = "/etc/passwd";
	char* sudoers_file = "/etc/sudoers";
	char* group_file = "/etc/group";
	char* shadow_file = "/etc/shadow";

	
	char c;
	int i;
	char buf[MAX_LENGTH];
	char **bptr;
	
	// Get the user
	int uid = getuid ();
	int passwd_fd = open (passwd_file);
	seek (passwd_fd, 0);
	char user_buf[MAX_LENGTH];
	char *user, *uid_passwd, *user_passwd, *uptr;

	for (;;)
		{
			uptr = user_buf;
			memset(user_buf, 0, MAX_LENGTH);

			for (i = 0; i < MAX_LENGTH; i++)
				{
					read (passwd_fd, &c, 1);
					if (c == '\n')
						break;
					memcpy (uptr, &c, 1);
					uptr++;
				}
			uptr = user_buf;
			char *buffer;
		 	bptr = &buffer;
			user = strtok_r (uptr, ":", bptr);
			user_passwd = strtok_r (NULL, ":", bptr);
			uid_passwd = strtok_r (NULL, ":", bptr);
			int uid_p = atoi (uid_passwd);
			if (uid_p == uid)
				break;
		
			while (c != '\n' && c != '\0')
				read (passwd_fd, &c, 1);

			if (read (passwd_fd, &c, 1) == 0 || c == '\0')
			{
				return -1; // Error if we don't find the user
			}

			seek (passwd_fd, tell (passwd_fd) - 1);
		}
	close (passwd_fd);

	/* Get the list of groups that are sudoers
		 File format: <group> <group> ... */
	int sudoers_fd = open (sudoers_file);
	seek (sudoers_fd, 0);
	char sudoers[MAX_LENGTH];
	memset(sudoers, 0, MAX_LENGTH);
	char* sptr = sudoers;

	for (i = 0; i < MAX_LENGTH; i++)
		{
			read (sudoers_fd, &c, 1);
			if (c == '\n')
				break;
			memcpy (sptr, &c, 1);
			sptr++;
		}
	sptr = sudoers;
	close (sudoers_fd);

	// Check to see if the user is in sudo groups
	int group_fd = open (group_file);
	seek (group_fd, 0);

	char* gptr;
	char* group_user;
	bool group_found = false;

	for (;;)
		{
			memset(buf, 0, MAX_LENGTH);
			gptr = buf;

			for (i = 0; i < MAX_LENGTH; i++)
				{
					read (group_fd, &c, 1);
					if (c == '\n')
						break;
					memcpy (gptr, &c, 1);
					gptr++;
				}
			gptr = buf;
			char *group_buf, *sudo_buf;
			bptr = &group_buf;
			char** sbptr = &sudo_buf;
			char* group = strtok_r (gptr, ":", bptr);
			// Loop through sudoers char *
			char* sudo_group = strtok_r (sptr, " ", sbptr);
			while (sudo_group != NULL)
				{
					if (!strcmp (group, sudo_group))
						break;
					sudo_group = strtok_r (NULL, " ", sbptr);
				}

			if (sudo_group != NULL)
				{
					strtok_r (NULL, ":", bptr); // group password
					strtok_r (NULL, ":", bptr); // group number
					char* group_users = strtok_r (NULL, ":", bptr);
					
					// Loop over group users to see if user is there
					char *user_buf;
					char **ubptr = &user_buf;

					group_user = strtok_r (group_users, " ", ubptr);
					while (group_user != NULL)
						{
							if (!strcmp(group_user, user))
								{
									group_found = true;
									break;
								}
							group_user = strtok_r (NULL, " ", ubptr);
						}
				}
			if (!group_found)
				{
					while (c != '\n' && c != '\0')
						read (group_fd, &c, 1);

					if (read (group_fd, &c, 1) == 0 || c == '\0')
						{
							printf("[sudo] requires privileges\n");
							return -1;
						}
				
					seek (group_fd, tell (group_fd) - 1);
				}
			else
				break;
		}
	close(group_fd);
	if (group_user == NULL)
		return -1;

	// Get password of user
	char password[20];
	char un[20];
	char hashed_pw[33];
	int attempts = 0;
	bool isHashed = false;
	char *pw = NULL;
	char *pw_attempt;

	if (!strcmp ("x", user_passwd))
		{
			isHashed = true;
			// Retrieve the hashed password
			int shadow_fd = open (shadow_file);
			
			char* shptr = "";
			
			for (;;)
				{
					memset(un, 0, 20);
					shptr = un;
					for (i = 0; i < 20; i++)
						{
							read (shadow_fd, &c, 1);
							if (c == ':')
								break;
							memcpy (shptr, &c, 1);
							shptr++;		
						}
					shptr = un;

					// Check to see if the user from shadow matches user
					if (!strcmp(user, shptr))
						{
							memset(hashed_pw, 0, 33);
							shptr = hashed_pw;

							for (i = 0; i < 33; i++)
								{
									read (shadow_fd, &c, 1);
									if (c == '\n')
										break;
									memcpy (shptr, &c, 1);
									shptr++;
								}

							pw = hashed_pw;
							break;
						}
					while (c != '\n' && c != '\0')
					read (shadow_fd, &c, 1);

					if (read (shadow_fd, &c, 1) == 0 || c == '\0')
						break;
		
					seek (shadow_fd, tell (shadow_fd) - 1);
				}

			close (shadow_fd);
		}
	else
		{
			pw = user_passwd;
		}
	if (pw == NULL)
		return -1;

	while (attempts < 3)
		{
			printf("[sudo] password for %s: ", user);
			read_line (password, sizeof password, true);
			
			if (isHashed)
				{
					// Hash password
					char hashed_password_attempt [33];
					md5 ((uint8_t*)password, strlen(password), hashed_password_attempt);
					pw_attempt = hashed_password_attempt;
				}
			else
				pw_attempt = password;
			if (!strcmp (pw, pw_attempt))
				{
					break;
				}
			else
				{
					printf("Sorry, try again.\n");
					attempts++;
					continue;
				}
		}
	if (attempts == 3)
		{
			printf("sudo: 3 incorrect password attempts\n");
			return -1;
		}

		// Figure out length of arguments
	int length = 0;
	for (i = 1; i < argc; i++)
		{
			length += strlen(argv[i]);
			// Add a space between arguments.
			if (i != argc - 1)
				length += 1;
		}

	// Impose some reasonable cmd limit
	if (length > MAX_LENGTH)
		return -1;

	char addr[MAX_LENGTH] = "\0";
	char* cmdline = addr;
	for (i = 1; i < argc; i++)
		{
			strlcat(cmdline, argv[i], MAX_LENGTH); 
			if (i != argc - 1)
				strlcat(cmdline, " ", MAX_LENGTH);
		}

	wait (exec (cmdline));
	return 0;
}
