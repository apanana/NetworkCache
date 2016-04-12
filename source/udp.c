#include "udp.h"

#define BUFFSIZE 1024

struct cache_obj{
    char * host;
    char * tcpport;
    char * udpport;
    struct addrinfo * tcpservinfo;
    struct addrinfo * udpservinfo;
};

////////////////////////////////////////////////////////////////////////
// UDP SETUP FUNCTIONS
////////////////////////////////////////////////////////////////////////
int udp_server_setup(char * UDPPORT){
	// Sets up a UDP socket and lets it listen for connections
	int udp_fd,status;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof(hints)); // clear out hints
	hints.ai_family = AF_UNSPEC; // unspecified IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // UDP
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((status = getaddrinfo(NULL,UDPPORT,&hints,&servinfo)) != 0){
		printf("UDP setup: getaddrinfo error: %s\n", gai_strerror(status));
		freeaddrinfo(servinfo);
		exit(1);
	}
    if ((udp_fd = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol)) == -1) {
		printf("UDP setup: socket error\n");
		close(udp_fd);
		freeaddrinfo(servinfo);
		exit(1);
	}
	//set socket timeout to 1000ms
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 50000;
	if (setsockopt(udp_fd, SOL_SOCKET, SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0) {
		printf("UDP setup: timeout error\n");
		close(udp_fd);
		freeaddrinfo(servinfo);
		exit(1);
	}
    if (bind(udp_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		printf("UDP setup: bind error\n");
		close(udp_fd);
		freeaddrinfo(servinfo);
		exit(1);
	}
	freeaddrinfo(servinfo);
	return udp_fd;
}

int connect_udp_server(cache_t cache){
	// Helps a client set up a connection to the UDP socket of the server specified in its cache.
    int sockfd;
    char s[INET6_ADDRSTRLEN];
    if ((sockfd = socket(cache->udpservinfo->ai_family, cache->udpservinfo->ai_socktype,cache->udpservinfo->ai_protocol)) == -1) {
        printf("UDP client: Socket Error.\n");
        exit(1);
    }
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 50000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0) {
		printf("UDP client: timeout error\n");
		close(sockfd);
		exit(1);
	}
    inet_ntop(cache->udpservinfo->ai_family, &(((struct sockaddr_in*)&cache->udpservinfo->ai_addr)->sin_addr), s, sizeof s);
    printf("UDP client: connecting to %s\n", s);
    return sockfd; 
}

////////////////////////////////////////////////////////////////////////
// UPP SEND/RECEIEVE
////////////////////////////////////////////////////////////////////////
char * receive_udp(int newfd,struct sockaddr_storage * source, socklen_t sin_size){
	// Receives a TCP message, combining packets of length = BUFFSIZE.
	int rec_len, 
		total = 0,
		resp_len = BUFFSIZE;
	char * buffer = calloc(BUFFSIZE,1);
	char * response = calloc(resp_len,1);
	char * temp;
	// We receieve packets until we see one terminated by "\n"
	do{
		if ((rec_len =recvfrom(newfd, buffer, BUFFSIZE, 0,(struct sockaddr_in*)source,&sin_size)) == -1){
			printf("UDP receive error\n");
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
		memset(buffer, '\0', sizeof(buffer));
	}
	while(buffer[rec_len-1]!='\n');
	printf("UDP: Received request: %s\n",response);
	return response;	
}

void send_udp(int newfd, char * out,struct sockaddr_storage * source, socklen_t sin_size){
	// Sends a UDP message, splitting it into packets of length = BUFFSIZE.
	printf("Sending: %s\n",out);
	int sent = 0,
		left = strlen(out);
	while (BUFFSIZE < left){
	    if (sendto(newfd, out, BUFFSIZE, 0,(struct sockaddr_in*)source,sin_size) == -1){
			printf("UDP send: error after %d bytes\n",sent);
	        close(newfd);
	        exit(0);
	    }
	    sent = sent + BUFFSIZE;
	    out = out + BUFFSIZE;
	    left = left - BUFFSIZE;
	}
	// Once a message is smaller than max-packet-size we send that.
    if (sendto(newfd, out, left, 0,(struct sockaddr_in*)source,sin_size) == -1){
		printf("UDP send: error after %d bytes\n",sent);
        close(newfd);
        exit(0);
    }
}

////////////////////////////////////////////////////////////////////////
// UDP REQUEST HANDLING
////////////////////////////////////////////////////////////////////////
void udp_server_request(int newfd, cache_t *c){
	// handles UDP server request by receiving a client's request, 
	// processing the request, and then responding to the client.
	struct sockaddr_storage client;
	socklen_t size = sizeof(client);

	// char * received = receive_udp(newfd,&client,size);
	// printf("UDP: Received request: %s\n",received);
	// char * request = process_request(c,received);
	int rec_len;
	char * buffer = calloc(BUFFSIZE,1);
	if ((rec_len =recvfrom(newfd, buffer, BUFFSIZE, 0,(struct sockaddr_in*)&client,&size)) == -1){
		printf("UDP receive error\n");
		close(newfd);
		exit(1);
	}
	printf("UDP: Received request: %s\n",buffer);
	char * request = process_request(c,buffer);


	printf("UDP: Request processed. Replying to client.\n");
	send_udp(newfd,request,&client,size);
	printf("UDP: Replied to client.\n");
}

char * udp_client_request(int serv_fd, char* request,cache_t cache){
	// handles UDP client request by sending and then receiving the server's response.
    send_udp(serv_fd,request,cache->udpservinfo->ai_addr,cache->udpservinfo->ai_addrlen);
	printf("UDP: Waiting for a response from server...\n");
	char * response = receive_udp(serv_fd,cache->udpservinfo->ai_addr,cache->udpservinfo->ai_addrlen);
	printf("UDP: Recieved response from server:\n%s\n",response);
	return response;
}