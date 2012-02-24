#ifndef _PROXY_SERVER_H
#define _PROXY_SERVER_H

#include <sys/types.h>

int init_server(int argc, char* argv[]);
void serve_demo(int sockfd);
void serve(int sockfd, size_t cpunum);

#endif /* _PROXY_SERVER_H */
