#include "tcp.h"

#define BUFFSIZE 100

char * receive_tcp(int newfd){
	int rec_len;
	char * buffer[BUFFSIZE];
	memset(buffer, '\0', sizeof(buffer));
	if ((rec_len =recv(newfd, buffer, BUFFSIZE-1, 0)) == -1){
		printf("receive error\n");
		close(newfd);
		exit(1);
	}
	buffer[rec_len] = '\0';
	printf("Recieved request: %s\n",buffer);
	return buffer;
}

void response_tcp(char * out,int newfd){
	printf("Sending response: %s\n",out);

    if (send(newfd, out, strlen(out), 0) == -1){
		printf("send error\n");
        close(newfd);
        exit(1);
        }

	if(strcmp(out,"204 No Content: Shutting down\n")==0){//shutdown
        if (send(newfd, "Closing connection!\n", 20, 0) == -1){
            printf("send error\n");
            close(newfd);
            exit(1);
        }
	}
}

int setup_tcp(char * TCPPORT){
	// Sets up a TCP socket and lets it listen for connections
	int tcp_fd, status;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof(hints)); // clear out hints
	hints.ai_family = AF_UNSPEC; // unspecified IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((status = getaddrinfo(NULL,TCPPORT,&hints,&servinfo)) != 0){
		printf("TCP setup: getaddrinfo error: %s\n", gai_strerror(status));
		freeaddrinfo(servinfo);
		exit(1);
	}
    if ((tcp_fd = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol)) == -1) {
		printf("TCP setup: socket error\n");
		freeaddrinfo(servinfo);
		exit(1);
	}
	// Allow socket to be reused
	int yes = 1;
    if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1){
        printf("TCP setup: setsockopt error\n");
    	close(tcp_fd);
        freeaddrinfo(servinfo);
        exit(1);
	}
    if (bind(tcp_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		printf("TCP setup: bind error\n");
		close(tcp_fd);
		freeaddrinfo(servinfo);
		exit(1);
	}
	freeaddrinfo(servinfo); // Don't need this anymore 
    if (listen(tcp_fd, 10) == -1) {
        printf("TCP setup: listen error\n");
        close(tcp_fd);     
        exit(1);
    }
	return tcp_fd;
}

void tcp_request(int newfd, cache_t *c){
	// handles TCP request by receiving, processing, and then responding.
	response_tcp(
		process_request(c,receive_tcp(newfd)),
		newfd);
}
	