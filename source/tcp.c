#include "tcp.h"

int setup_tcp(char * TCPPORT){
	int tcp_fd;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof(hints)); // clear out hints
	hints.ai_family = AF_UNSPEC; // unspecified IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE; // use my IP

	if (getaddrinfo(NULL,TCPPORT,&hints,&servinfo) != 0){
		printf("getaddrinfo error\n");
		return 1;
	}
    if ((tcp_fd = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol)) == -1) {
		printf("socket error\n");
		return 1;
	}
	//Reuse an old socket
	int yes = 1;
    if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
        printf("setsockopt error\n");
	}
    if (bind(tcp_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		close(tcp_fd);
		printf("bind error\n");
		return 1;
	}
	freeaddrinfo(servinfo); // Don't need this anymore 
    if (listen(tcp_fd, BACKLOG) == -1) {
        printf("listen error\n");        
        return 1;
    }
	return tcp_fd;
}

void tcp_request(int newfd, cache_t *c){
	int rec_len;
	char * buffer[BUFFSIZE];
	char * out[BUFFSIZE];
	memset(buffer, '\0', sizeof(buffer));
	if ((rec_len =recv(newfd, buffer, BUFFSIZE-1, 0)) == -1){
		printf("receive error\n");
		close(newfd);
		exit(1);
	}
	buffer[rec_len] = '\0';
	printf("Recieved request: %s\n",buffer);
	printf("THIS IS THE PRE-REQUEST POINTER %p\n\n\n",c);
	strcpy(out,process_request(c,buffer,rec_len));// need a better way of doing this lol
	printf("THIS IS THE POST-REQUEST POINTER %p\n\n\n",c);
	printf("Sending response: %s\n",out);

    if (send(newfd, out, strlen(out), 0) == -1){
		printf("send error\n");
        close(newfd);
        exit(1);
        }

	if(strcmp(out,"204 No Content: Shutting down")==0){//shutdown
        if (send(newfd, "Closing connection!\n", 20, 0) == -1){
            printf("send error\n");
            close(newfd);
            exit(1);
        }
	}
	// return c;
}
	