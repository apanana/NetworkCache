#include "tcp.h"

#define BUFFSIZE 100

struct cache_obj{
    char * host;
    char * tcpport;
    char * udpport;
    struct addrinfo * tcpservinfo;
    struct addrinfo * udpservinfo;
};

int connect_tcp_server(cache_t cache){
    int sockfd;
    char s[INET6_ADDRSTRLEN];
    if ((sockfd = socket(cache->tcpservinfo->ai_family, cache->tcpservinfo->ai_socktype,cache->tcpservinfo->ai_protocol)) == -1) {
        printf("TCP client: Socket Error.\n");
        exit(1);
    }
    if (connect(sockfd, cache->tcpservinfo->ai_addr, cache->tcpservinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("TCP client: Connect Error.\n");
        exit(1);
    }
    inet_ntop(cache->tcpservinfo->ai_family, &(((struct sockaddr_in*)&cache->tcpservinfo->ai_addr)->sin_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    return sockfd;
}

char * send_rec_tcp(int sockfd, char* buf){
    int numbytes;
    if ((numbytes = send(sockfd, buf, strlen(buf), 0)) == -1) {
        perror("");
        exit(1);
    }
    char * buffer[BUFFSIZE];
    int rec_len;
    memset(&buffer, '\0', sizeof(buffer));
    if ((rec_len =recv(sockfd, buffer, BUFFSIZE-1, 0)) == -1){
        printf("TCP client: receive error\n");
        close(sockfd);
        exit(1);
    }
    buffer[rec_len] = '\0';
    printf("Recieved response: %s\n",buffer);
    return buffer;
}

int setup_tcp_server(char * TCPPORT){
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
		close(tcp_fd);
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
	freeaddrinfo(servinfo);
    if (listen(tcp_fd, 10) == -1) {
        printf("TCP setup: listen error\n");
        close(tcp_fd);     
        exit(1);
    }
	return tcp_fd;
}

char * receive_tcp(int newfd){
	int rec_len, 
		total = 0,
		resp_len = BUFFSIZE;
	char buffer[BUFFSIZE];
	memset(buffer, '\0', sizeof(buffer));
	char * response = calloc(resp_len,1);
	char * temp;
	do{
		if ((rec_len =recv(newfd, buffer, BUFFSIZE, 0)) == -1){
			printf("receive error\n");
			close(newfd);
			exit(1);
		}
		if (rec_len + total > resp_len){
			temp = calloc(resp_len + BUFFSIZE,1);
			memcpy(temp,response,resp_len);
			free(response);
			response = temp;
			resp_len = resp_len + rec_len;
		}
		strcat(response,buffer);
		total = total + rec_len;
	}
	while(buffer[rec_len-1]!='\n');
	printf("Recieved request: %s\n",response);
	return response;	
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

void tcp_server_request(int newfd, cache_t *c){
	// handles TCP request by receiving, processing, and then responding.
	response_tcp(
		process_request(c,receive_tcp(newfd)),
		newfd);
}
	