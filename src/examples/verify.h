#ifndef VERIFY_H
#define VERIFY_H

bool verify (char *username, char *userpasswd, int passwd_fd, int shadow_fd, char *uid, char *gid);

#endif
