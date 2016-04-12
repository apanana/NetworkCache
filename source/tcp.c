#include "tcp.h"

#define BUFFSIZE 1024

struct cache_obj{
    char * host;
    char * tcpport;
    char * udpport;
    struct addrinfo * tcpservinfo;
    struct addrinfo * udpservinfo;
};

////////////////////////////////////////////////////////////////////////
// TCP SETUP FUNCTIONS
////////////////////////////////////////////////////////////////////////
int tcp_server_setup(char * TCPPORT){
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

int connect_tcp_server(cache_t cache){
	// Helps a client set up a connection to the TCP socket of the server specified in its cache.
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
    printf("TCP client: connecting to %s\n", s);
    return sockfd;
}

////////////////////////////////////////////////////////////////////////
// TCP SEND/RECEIEVE
////////////////////////////////////////////////////////////////////////
char * receive_tcp(int newfd){
	// Receives a TCP message, combining packets of length = BUFFSIZE.
	int rec_len, 
		total = 0,
		resp_len = BUFFSIZE;
	char buffer[BUFFSIZE];
	memset(buffer, '\0', sizeof(buffer));
	char * response = calloc(resp_len,1);
	char * temp;
	// We receieve packets until we see one terminated by "\n"
	do{
		if ((rec_len =recv(newfd, buffer, BUFFSIZE, 0)) == -1){
			printf("TCP: receive error\n");
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
	printf("TCP: Received request: %s\n",response);
	return response;	
}

void send_tcp(int newfd,char * out){
	// Sends a TCP message, splitting it into packets of length = BUFFSIZE.
	printf("Sending: %s\n",out);
	int sent = 0,
		left = strlen(out);
	while (BUFFSIZE < left){
	    if (send(newfd, out, BUFFSIZE, 0) == -1){
			printf("TCP send: error after %d bytes\n",sent);
	        close(newfd);
	        exit(1);
	        }
	    sent = sent + BUFFSIZE;
	    out = out + BUFFSIZE;
	    left = left - BUFFSIZE;
	}
	// Once a message is smaller than max-packet-size we send that.
    if (send(newfd, out, left, 0) == -1){
		printf("TCP send: error after %d bytes\n",sent);
	    close(newfd);
	    exit(1);
    }
}

////////////////////////////////////////////////////////////////////////
// TCP REQUEST HANDLING
////////////////////////////////////////////////////////////////////////

void tcp_server_request(int newfd, cache_t *c){
	// handles TCP server request by receiving a client's request, 
	// processing the request, and then responding to the client.
	char * received = receive_tcp(newfd);
	printf("TCP: Received request: %s\n",received);
	char * request = process_request(c,received);
	printf("TCP: Request processed. Replying to client.\n");
	send_tcp(newfd,request);
	printf("TCP: Replied to client.\n");
}

char * tcp_client_request(int serv_fd, char* request){
	// handles TCP client request by sending and then receiving the server's response.
	send_tcp(serv_fd,request);
	printf("TCP: Waiting for a response from server...\n");
	char * response = receive_tcp(serv_fd);
	printf("TCP: Recieved response from server:\n%s\n",response);
	return response;
}
	