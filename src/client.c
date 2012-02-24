#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>

#define BUFLEN   128
#define MAXSLEEP 16

int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen)
{
	int nsec;

	/* Try to connect with exponential backoff. */

	for (nsec = 1; nsec <= MAXSLEEP; nsec <<=1){
		if (connect(sockfd, addr, alen) == 0)
			return 0;    /* connection accepted */

		/* Delay before trying again. */
		if (nsec <= (MAXSLEEP / 2))
			sleep(nsec);
	}
	return -1;
}

int main(int argc, char *argv[])
{
	struct addrinfo *ailist, *aip;
	struct addrinfo  hint;
	int              sockfd, err, cnt;
	size_t count, slen;

	if (argc != 3){
		fprintf(stderr, "usage: client hostname string\n");
		return EXIT_FAILURE;
	}

	slen = strlen(argv[2]);
	hint.ai_flags = 0;
	hint.ai_family = 0;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;

	if ((err = getaddrinfo(argv[1], "tecent", &hint, &ailist)) != 0){
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
		return EXIT_FAILURE;
	}

	for (aip = ailist; aip != NULL; aip = aip->ai_next){
		if ((sockfd = socket(aip->ai_family, SOCK_STREAM, 0)) < 0)
			err = errno;
		if (connect_retry(sockfd, aip->ai_addr, aip->ai_addrlen) < 0){
			err = errno;
		}else{
			/* printf("slen = %d\n", slen); */	
			for (count = 0; count < slen; ){
				if ((cnt = send(sockfd, argv[2]+count, slen-count, 0)) != -1){
					/* printf("cnt = %d\n", cnt); */
					count += cnt;
				}else{
					fprintf(stderr,
					        "sending warning : %d bytes sent of %d bytes to be sent.\n",
					        count, slen);
					return EXIT_FAILURE;
				}
			}
			return EXIT_SUCCESS;
		}
	}
	fprintf(stderr, "can't connect to %s : %s.\n", argv[1], strerror(errno));
	return EXIT_FAILURE;
}
