#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#define NUM_TCP_KEEPCNT 3
#define NUM_TCP_KEEPIDLE 10
#define NUM_TCP_KEEPINTVL 10

/**
 * Basic client,
 * 	reads and prints out everything server sends it.
 **/
int keepalive(int s){
	int n;
	int r;
	int optval;
	socklen_t optlen = sizeof(optval);

	/* Set the option active */
	optval = 1;
	r = setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
	assert(-1 != r);

	/* Check the status again */
	r = getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
	assert(-1 != r);
	fprintf(stderr, "<K> SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

	n = NUM_TCP_KEEPCNT;
	r = setsockopt(s, SOL_TCP, TCP_KEEPCNT, (char*)&n, sizeof(n));
	assert(-1 != r);

	r = getsockopt(s, SOL_TCP, TCP_KEEPCNT, &optval, &optlen);
	assert(-1 != r);
	fprintf(stderr, "<K> TCP_KEEPCNT is %d\n", optval);

	/* set up TCP_KEEPIDLE */
	n = NUM_TCP_KEEPIDLE;
	r = setsockopt(s, SOL_TCP, TCP_KEEPIDLE, (char*)&n, sizeof(n));
	assert(-1 != r);

	r = getsockopt(s, SOL_TCP, TCP_KEEPIDLE, &optval, &optlen);
	assert(-1 != r);
	fprintf(stderr, "<K> TCP_KEEPIDLE is %d\n", optval);

	/* set up TCP_KEEPINTVL */
	n = NUM_TCP_KEEPINTVL;
	r = setsockopt(s, SOL_TCP, TCP_KEEPINTVL, (char*)&n, sizeof(n));
	assert(-1 != r);

	r = getsockopt(s, SOL_TCP, TCP_KEEPINTVL, &optval, &optlen);
	assert(-1 != r);
	fprintf(stderr, "<K> TCP_KEEPINTVL is %d\n", optval);

	return 1;
}

int main(int argc, char ** argv) {

	if(argc!=3) {
		fprintf(stderr,"Usage: client host port\n");
		return 2;
	}

	struct addrinfo hints={0};		// zero'd out addrinfo structure
	hints.ai_family = AF_UNSPEC; 	// IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;// TCP

	struct addrinfo *resolved_addrs;
	const int err_val = getaddrinfo(argv[1],argv[2],&hints,&resolved_addrs);
	if(err_val!=0) {
		fprintf(stderr,"resolve address info: %s\n",gai_strerror(err_val));
		return 2;
	}

	struct addrinfo *p;
	int fd;
	for(p=resolved_addrs; p!=NULL; p = p->ai_next) {
		fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(fd < 0) {
			perror("Establish socket:");
		}
		else {
			if(connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
				perror("connect");
				close(fd);
			}
			else { // Working socket obtained, leave loop.
				break;
			}
		}
	}
	keepalive(fd);
	if(p==NULL) { // Failed to bind to any address, die horribly.
		fprintf(stderr,"Failed to bind\n");
		return 2;
	}

	int num_bytes;
	char buf[256];
	while( (num_bytes = recv(fd,buf,sizeof(buf)-1,0)) != 0 ) { // while connection open.
		if(num_bytes==-1) {
			perror("recv");
			close(fd);
			return 2;
		}
		buf[num_bytes]='\0';
		printf("%s",buf);
		break;
	}
	char test[] = "test ";
	int i = 0;
	char time[33];
	while(1){
		sprintf(time, "%d", i);
		char *msg = (char *)calloc(strlen(test) + strlen(time) + strlen("\r\n"), sizeof(char));
		strcat(msg, test);
		strcat(msg, time);
		strcat(msg, "\r\n");
		sleep(1);
		num_bytes = send(fd, msg, strlen(msg), 0);
		if(num_bytes > 0){
			fprintf(stderr, "Sending:%s", msg);
			i++;
		}else{
			perror("send");
			close(fd);
			exit(1);
		}
	}
}
