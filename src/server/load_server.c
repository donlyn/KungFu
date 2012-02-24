#include <sys/epoll.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "channel.h"
#include "helper.h"

#define MAXEVENTS 10

int main(int argc, char *argv[])
{
	const size_t buflen = 128;
	char *buf[buflen];
	struct epoll_event ev, events[MAXEVENTS];
	int epollfd, nfds, ifd, clfd, cnt, flags;
	size_t linkcount = 1;
	

	if ((epollfd = epoll_create(1)) == -1)
		err_sys("epoll_create error : ");
	
	ev.events  = EPOLLIN;
	ev.data.fd = STDIN_FILENO;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1)
		err_sys("epoll_ctl for stdin error : ");

	for ( ; ; ){
		nfds = epoll_wait(epollfd, events, 10, -1);
		if (nfds == -1)
			err_sys("epoll_wait error : ");

		for (ifd = 0; ifd < nfds; ifd++){
			if (events[ifd].data.fd == STDIN_FILENO){
				clfd = recv_fd(STDIN_FILENO);
				if (clfd == -1)
					err_quit("recv_fd error");
				if ((flags = fcntl(clfd, F_GETFD, 0)) == -1)
					err_quit("fcntl getting flags error");
				if (fcntl(clfd, F_SETFD, (flags & O_NONBLOCK)) == -1)
					err_quit("fcntl setting flags error");
				ev.events = EPOLLIN;
				ev.data.fd = clfd;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clfd, &ev) == -1)
					err_sys("epoll_ctl for client fd : ");
			}else{
				while ((cnt = recv(events[ifd].data.fd, buf, buflen, 0)) > 0)
					write(STDOUT_FILENO, buf, cnt);
				if (cnt == 0){
					if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[ifd].data.fd, NULL) == -1)
						err_sys("epoll_ctl for client fd : ");
					close(events[ifd].data.fd);
					fprintf(stdout, "\t pid : %d\t linkcount : %d\n", getpid(), linkcount++);
				}
				/* write(STDOUT_FILENO, "\n", 1); */
				if (cnt < 0)
					err_sys("recv error");
			}
		}
	}
}
