#include "proxy_server.h"
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "helper.h"
#include "channel.h"

#define QLEN 128

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

/*
int init_server(int type, const struct sockaddr *addr, socklen_t alen, int qlen)
{
	int fd, err;
	int reuse = 1;

	if ((fd = socket(addr->sa_family, type, 0)) < 0)
		return -1;
	
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0){
		err = errno;
		goto errout;
	}

	if (bind(fd, addr, alen) < 0){
		err = errno;
		goto errout;
	}

	if (type == SOCK_STREAM || type == SOCK_SEQPACKET){
		if (listen(fd, qlen) < 0){
			err = errno;
			goto errout;
		}
	}
	return fd;

errout:
	close(fd);
	errno = err;
	return -1;
}
*/

int init_server(int argc, char* argv[])
{
	struct addrinfo *ailist, *aip;
	struct addrinfo  hint;
	int              sockfd, err, n;
	char            *host;
	int              reuse = 1;

	if (argc != 1)
		err_quit("usage: tecentd");

#ifdef _SC_HOST_NAME_MAX
	n = sysconf(_SC_HOST_NAME_MAX);
	if (n < 0)
#endif
		n = HOST_NAME_MAX;
	host = malloc(n);
	if (host == NULL)
		err_sys("malloc error");
	if (gethostname(host, n) < 0)
		err_sys("gethostname error");
	/* daemonize("tecentd"); */
	hint.ai_flags = AI_CANONNAME;
	hint.ai_family = 0;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;
	
	if ((err = getaddrinfo(host, "tecent", &hint, &ailist)) != 0){
		fprintf(stderr, "tecentd: getaddrinfo error: %s", gai_strerror(err));
		exit(1);
	}

	for (aip = ailist; aip != NULL; aip = aip->ai_next) {
		if ((sockfd = socket(aip->ai_addr->sa_family, SOCK_STREAM, 0)) < 0)
			return -1;
		
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0){
			err = errno;
			goto errout;
		}

		if (bind(sockfd, aip->ai_addr, aip->ai_addrlen) < 0){
			err = errno;	
			goto errout;
		}

		if (listen(sockfd, QLEN) < 0){
			err = errno;
			goto errout;
		}
		return sockfd;
	}

errout:
	close(sockfd);
	errno = err;
	return -1;
}

void serve_demo(int sockfd)
{
	const size_t buflen = 128;
	int clfd, cnt;
	char buf[buflen];

	for ( ; ; ){
		clfd = accept(sockfd, NULL, NULL);
		if (clfd < 0){
			fprintf(stderr, "tecentd: accept error: %s", strerror(errno));
			exit(1);
		}

		while ((cnt = recv(clfd, buf, buflen, 0)) > 0)
			write(STDOUT_FILENO, buf, cnt);
		write(STDOUT_FILENO, "\n", 1);
		if (cnt < 0)
			err_sys("recv error");
	}
}

void serve(int sockfd, size_t cpunum)
{
	pid_t *pid;
	int  clfd;
	size_t cpu;
	size_t loadserver = 0;
	int (*channels)[2];
	
	channels = (int (*)[2])malloc(cpunum*sizeof(int)*2);
	pid      = (pid_t *)malloc(cpunum*sizeof(pid_t));

	for (cpu = 0; cpu < cpunum; ++cpu){
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, channels[cpu]) == -1){
			fprintf(stderr, "socketpair error\n");
			exit(1);
		}
		if ((pid[cpu] = fork()) == -1){
			fprintf(stderr, "fore error!\n");
			exit(1);
		} 
		
		if(pid[cpu] == 0){               /* child */
			close(channels[cpu][0]);
			if (channels[cpu][1] != STDIN_FILENO &&
				dup2(channels[cpu][1], STDIN_FILENO) != STDIN_FILENO)
				err_sys("dup2 to stdin error");
			/*
			if (channels[cpu][1] != STDOUT_FILENO &&
				dup2(channels[cpu][1], STDOUT_FILENO) != STDOUT_FILENO)
				err_sys("dup2 to stdout error");
			*/
			if (execl("./load_server", "load_server", (char*)0) < 0)
				err_sys("execl error");
		}
		close(channels[cpu][1]);    /* parent */
	}
	
	for ( ; ; ){
		clfd = accept(sockfd, NULL, NULL);
		if (clfd < 0){
			fprintf(stderr, "tecentd: accept error: %s", strerror(errno));
			break;;
		}
		if (send_fd(channels[loadserver++][0], clfd) == -1){
			fprintf(stderr, "send_fd : sending file descriptor error");
			break;
		}
		loadserver %= cpunum;
		close(clfd);
	}

	for (cpu = 0; cpu < cpunum; ++cpu){
		kill(pid[cpu], SIGKILL);
	}
}

