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

int keepalive(int s){
	int n;
	int r;
	int optval;
	socklen_t optlen = sizeof(optval);

	/* set up SO_KEEPALIVE */
	optval = 1;
	r = setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
	assert(-1 != r);

	r = getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
	assert(-1 != r);
	fprintf(stderr, "<K> SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

	/* set up TCP_KEEPCNT */
	n = NUM_TCP_KEEPCNT;
	r = setsockopt(s, SOL_TCP, TCP_KEEPCNT, (char*)&n, sizeof(n));
	assert(-1 != r);

	r = getsockopt(s, SOL_TCP, TCP_KEEPCNT, &optval, &optlen);
	assert(-1 != r);
	fprintf(stderr, "<K> TCP_KEEPCNT is %d\n", (optval ? "ON" : "OFF"));

	/* set up TCP_KEEPIDLE */
	n = NUM_TCP_KEEPIDLE;
	r = setsockopt(s, SOL_TCP, TCP_KEEPIDLE, (char*)&n, sizeof(n));
	assert(-1 != r);

	r = getsockopt(s, SOL_TCP, TCP_KEEPIDLE, &optval, &optlen);
	assert(-1 != r);
	fprintf(stderr, "<K> TCP_KEEPIDLE is %d\n", (optval ? "ON" : "OFF"));

	/* set up TCP_KEEPINTVL */
	n = NUM_TCP_KEEPINTVL;
	r = setsockopt(s, SOL_TCP, TCP_KEEPINTVL, (char*)&n, sizeof(n));
	assert(-1 != r);

	r = getsockopt(s, SOL_TCP, TCP_KEEPINTVL, &optval, &optlen);
	assert(-1 != r);
	fprintf(stderr, "<K> TCP_KEEPINTVL is %d\n", (optval ? "ON" : "OFF"));

	return 1;
}

int main() {
	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	const char * PORT = "5555";
	fprintf(stderr, "Hostname:%s \tPort:%s\n", hostname, PORT);

	struct addrinfo hints;  // Attributes desired for sockets filled by getaddrinfo.
	struct addrinfo *addrs; // LinkedList of addresses matching parameters and hints.
	struct addrinfo *p;     // used to iterate over addrs
	int listen_fd;          // File descriptor to listen for new connections.
	int yes=1;              // Setting socket options requires a pointer to the value
	// you are setting it to, need a memory address with 'true'.
	int err_val;            // Temparary storage for error values where necessary

	// Provide properties for address
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;		// IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;	// TCP
	hints.ai_flags = AI_PASSIVE;		// Use local machine addr for host

	// Link addrs to an address info list based on hints.
	err_val = getaddrinfo(NULL,PORT,&hints,&addrs);
	if(err_val!=0) {
		fprintf(stderr,"Resolve listener: %s", gai_strerror(err_val));
		return 2;
	}


	for(p=addrs; p!=NULL; p = p->ai_next) {
		// Get file descriptor for accepting new connections.
		listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(listen_fd == -1) {
			perror("Establish socket");
		}
		else {
			// Make it so port can be reused immediatly upon close restart.
			if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))==-1) {
				perror("Reusable socket");
			}
			// Bind socket to the port specified before.
			if(bind(listen_fd, p->ai_addr, p->ai_addrlen) < 0) {
				perror("Bind");
			}
			else {
				// succesfully setup socket binding, Exit.
				break;
			}
		}
	}
	keepalive(listen_fd);
	if(p==NULL) {
		// Failed to bind to any address, die horribly.
		fprintf(stderr,"Failed to bind\n");
		return 2;
	}

	// free memory allocated for address info list.
	freeaddrinfo(addrs);

	// Listen for connections, wait on up to 7 at a time.
	err_val = listen(listen_fd, 7);
	if( err_val==-1 ) {
		perror("Listen");
		return 2;
	}

	// Accept one connection at a time. Blocks until connect.
	while(1) {
		int fd = accept(listen_fd,NULL,NULL);
		if(fd==-1) {
			perror("accept");
		}
		else {
			// Send connection msg.
			// Send/Recv (and open/write) are not guaranteed to send/recieve full msg,
			//	this is not accounted for.
			char *connected = (char *)calloc(strlen("Connected to ") + strlen(hostname) + strlen("\r\n"), sizeof(char));
			strcat(connected, "Connected to ");
			strcat(connected, hostname);
			strcat(connected, "\r\n");
			send(fd,connected,strlen(connected),0);
			char buf[256];
			int num_bytes;
			while(1){
				num_bytes = recv(fd, buf, sizeof(buf)-1,0);
				if(num_bytes > 0){
					buf[num_bytes] = '\0';
					printf("%s", buf);
				}else{
					perror(NULL);
					close(fd);
					break;
				}
			}
		}
	}
	close(listen_fd);
	return 0;
}
