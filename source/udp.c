# include "udp.h"

#define MAXLINE 100

int setup_udp(char * UDPPORT){
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
	//Reuse an old socket
	int yes = 1;
    if (setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1){
        printf("UDP setup: setsockopt error\n");
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
	return udp_fd;
}

char * receive_udp(int newfd,struct sockaddr_storage * source){
	socklen_t sin_size = sizeof(* source);
	int rec_len, 
		total = 0,
		resp_len = BUFFSIZE;
	char buffer[BUFFSIZE];
	memset(buffer, '\0', sizeof(buffer));
	char * response = calloc(resp_len,1);
	char * temp;
	do{
		if ((rec_len =recvfrom(newfd, buffer, BUFFSIZE-1, 0,(struct sockaddr_in*)source,&sin_size)) == -1){
			printf("UDP receive error\n");
			close(newfd);
			exit(1);
		}
		if (buffer == NULL){
			printf("No UDP Request\n");
			close(newfd);
			return NULL;
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
		printf("TOTAL IS: %d\n",total);
	}
	while(buffer[rec_len-1]!='\n');

	printf("Recieved request: %s\n",response);
	printf("REQUEST LENGTH IS: %d\n",strlen(response));
	return response;	
}

// void response_udp(char * out,int newfd,struct sockaddr_storage * source,int sin_size){
// 	// socklen_t sin_size = sizeof(* source);
// 	if (out == NULL) exit(0);
// 	printf("Sending response: %s\n",out);
// 	printf("RESPONSE SIZE IS: %d\n",strlen(out));
// 	printf("RESPONSE POINTER IS: %p\n",out);
// 	printf("FD IS: %d\n",newfd);
// 	printf("SEND 333;;;EXTADDR IS %p\n",source);
// 	char buffer[strlen(out)-1];
// 	strcpy(buffer,out);
// 	printf("BUFFER IS: %s\n",buffer);
// 	printf("BUFFER LENGTH IS; %d\n",strlen(buffer));
//     // if (sendto(newfd, out, strlen(out), 0,(struct sockaddr_in*)source,sin_size) == -1){
//     if (sendto(newfd, buffer, strlen(buffer), 0,(struct sockaddr_in*)source,sin_size) == -1){
// 		printf("UDP: send error\n");
//         close(newfd);
//         exit(0);
//     }
// }

// void udp_request(int newfd, cache_t *c){
// 	struct sockaddr_storage ext_addr;
// 	int size = sizeof(ext_addr);
// 	printf("UDP PRE; EXTADDR IS %p\n",&ext_addr);
// 	printf("UDP PRE FD: %d\n",newfd);
// 	char * request = receive_udp(newfd,&ext_addr);
// 	printf("REQUEST IS NOW : %s\n",request);
// 	printf("UDP REQ - REQUEST POINTER: %p\n",request);
// 	if (request == NULL) exit(0);
// 	char * process = process_request(c,request);
// 	printf("UDP REQ - PROCESS POINTER: %p\n",process);
// 	printf("PROCESS IS NOW : %s\n",process);
// 	response_udp(process,newfd,&ext_addr,size);
// 	// handles UDP request by receiving, processing, and then responding.
// }

void udp_request(int newfd, cache_t *c){
	struct sockaddr_storage ext_addr;
	socklen_t sin_size = sizeof(ext_addr);
	char * out[BUFFSIZE];
	int rec_len;
	char * buffer[BUFFSIZE];
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
	strcpy(out,process_request(c,buffer));// need a better way of doing this lol

    if (sendto(newfd, out, strlen(out), 0,(struct sockaddr_in*)&ext_addr,sin_size) == -1){
		printf("send error\n");
        close(newfd);
        exit(0);
        }

}