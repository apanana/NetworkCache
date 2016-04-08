# include "udp.h"

int setup_udp(char * UDPPORT){
	int udp_fd;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof(hints)); // clear out hints
	hints.ai_family = AF_UNSPEC; // unspecified IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // TCP
	hints.ai_flags = AI_PASSIVE; // use my IP
	if (getaddrinfo(NULL,UDPPORT,&hints,&servinfo) != 0){
		printf("getaddrinfo error\n");
		return 1;
	}
    if ((udp_fd = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol)) == -1) {
		printf("socket error\n");
		return 1;
	}
	//Reuse an old socket
	int yes = 1;
    if (setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
        printf("setsockopt error\n");
	}
    if (bind(udp_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		close(udp_fd);
		printf("bind error\n");
		return 1;
	}
	return udp_fd;
}

void udp_request(int newfd, cache_t *c){
	struct sockaddr_storage ext_addr;
	socklen_t sin_size = sizeof(ext_addr);
	int rec_len;
	char * buffer[BUFFSIZE];
	char * out[BUFFSIZE];
	memset(buffer, '\0', sizeof(buffer));

	if ((rec_len =recvfrom(newfd, buffer, BUFFSIZE-1, 0,(struct sockaddr_in*)&ext_addr,&sin_size)) == -1){
		printf("UDP receive error\n");
		close(newfd);
		exit(1);
	}

	buffer[rec_len] = '\0';
	if (buffer == NULL){
		printf("No UDP Request\n");
		close(newfd);
		exit(0);
	}

	strcpy(out,process_request(c,buffer,rec_len));// need a better way of doing this lol

    if (sendto(newfd, out, strlen(out), 0,(struct sockaddr_in*)&ext_addr,sin_size) == -1){
		printf("send error\n");
        close(newfd);
        exit(0);
        }

	if(strcmp(out,"204 No Content: Shutting down")==0){//shutdown
        if (send(newfd, "Closing connection!\n", 20, 0) == -1){
            printf("send error\n");
            close(newfd);
            exit(1);
        }
	}
}