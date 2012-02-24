#include <stdio.h>
#include <stdlib.h>
#include "proxy_server.h"

int main(int argc, char *argv[])
{
	int sockfd;

	if ((sockfd = init_server(argc, argv)) < 0){
		fprintf(stderr, "initialing server fails\n");
		return EXIT_FAILURE;
	}
	//serve_demo(sockfd);
	serve(sockfd, 4);
}

