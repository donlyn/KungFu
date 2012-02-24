#include "channel.h"
#include <stdio.h>
#include <sys/socket.h>
#include "helper.h"

#define CONTROLLEN CMSG_SPACE(sizeof(int))
#define CMSGLEN    CMSG_LEN(sizeof(int))

int send_fd(int fd, int fd_to_send)
{
	static struct cmsghdr *cmptr = NULL;  /* malloc'ed first time */
	
	struct iovec   iov[1];
	struct msghdr  msg;
	char           buf[2];            /* send_fd()/recv_fd() 2-byte protocol */

	iov[0].iov_base = buf;
	iov[0].iov_len  = 2;
	msg.msg_iov     = iov;
	msg.msg_iovlen = 1;
	msg.msg_name    = NULL;
	msg.msg_namelen = 0;
	if (fd_to_send < 0){
		return -1;
	}else{
		if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
			return -1;
		cmptr->cmsg_level   = SOL_SOCKET;
		cmptr->cmsg_type    = SCM_RIGHTS;
		cmptr->cmsg_len     = CMSGLEN; // CONTROLLEN;
		msg.msg_control     = cmptr;
		msg.msg_controllen  = CONTROLLEN;
		*(int *)CMSG_DATA(cmptr) = fd_to_send;
		buf[1] = 0;
	}
	buf[0] = 0;
	if (sendmsg(fd, &msg, 0) != 2)
		return -1;
	return 0;
}

int recv_fd(int fd)
{
	static struct cmsghdr *cmptr = NULL;  /* malloc'ed first time */
	
	int            newfd, nr, status;
	char          *ptr;
	char           buf[MAXLINE];
	struct iovec   iov[1];
	struct msghdr  msg;
	status = -1;

	for ( ; ;){
		iov[0].iov_base = buf;
		iov[0].iov_len  = sizeof(buf);
		msg.msg_iov     = iov;
		msg.msg_iovlen  = 1;
		msg.msg_name    = NULL;
		msg.msg_namelen = 0;
		if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
			return -1;
		msg.msg_control = cmptr;
		msg.msg_controllen = CONTROLLEN;
		if ((nr = recvmsg(fd, &msg, 0)) < 0){
			err_sys("recvmsg error");
		}else if (nr == 0) {
			err_ret("connection closed");
			return -1;
		}
	    //fprintf(stdout, "nr = %d, CONTROLLEN = %d, msg.msg_controllen = %d\n", nr, CONTROLLEN, msg.msg_controllen);
		for (ptr = buf; ptr < &buf[nr]; ) {
			if (*ptr++ == 0){
				if (ptr != &buf[nr-1])
					err_quit("message format error");
				status = *ptr & 0xFF;   /* prevent sign extension */
				if (status == 0){
					if (msg.msg_controllen != CONTROLLEN)
						err_quit("length of message received is invalid");
					newfd = *(int*)CMSG_DATA(cmptr);
				}else {
					newfd = -status;
				}
				nr -=2;
			}
		}
		if (nr > 0) return -1;
		if (status >= 0) return newfd;
	}
}

int send_err(int fd, int status, const char *errmsg)
{
	return 0;
}

