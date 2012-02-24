#ifndef _CHANNEL_H
#define _CHANNEL_H

#include <sys/types.h>

int send_fd(int fd, int fd_to_send);
int send_err(int fd, int status, const char *errmsg);
int recv_fd(int fd);

#endif /* _CHANNEL_H */
