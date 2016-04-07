#include "request.c"
#include <stdbool.h>
// Parsing command line inputs
#include <unistd.h>
#include <errno.h>
// Network Libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BACKLOG 10
#define BUFFSIZE 100

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
}

int main(int argc, char *argv[]){
	////////////////////////////////////////////////////////////////////////
	char * TCP = "3490";
	char * UDP = "4590";
	int MAXMEM = 1000;
	bool SHUTDOWN = false;
	bool INITIAL = true;
	// PARSING COMMAND LINE INPUTS
	// Get maxmem and portnum from command line
	char *ptr;
	int opt;
	// Using getopt to parse the command line arguments
	while ((opt = getopt (argc, argv, "m:t:u:")) != -1){
		switch(opt){
			case 'm':
				MAXMEM = strtol(optarg,&ptr,10);
				break;
			case 't':
				TCP = optarg;
				break;
			case 'u':
				UDP = optarg;
				break;
			case '?':
				if (optopt == 'm' || optopt == 't' || optopt == 'u')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr,
						"Unknown option character `\\x%x'.\n",
						optopt);
				return 1;
		  	default:
				abort();
		}
	}
	printf("Initial settings: maxmem = %d, TCP = %s, UDP = %s\n",MAXMEM,TCP,UDP);

	////////////////////////////////////////////////////////////////////////
	// SETTING UP OUR SERVER	
	int tcp_fd = setup_tcp(TCP);
	int udp_fd = setup_udp(UDP);

    ////////////////////////////////////////////////////////////////////////
	// Getting server ready for a connection
	int newfd, rec_len;
	struct sockaddr_storage ext_addr;
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];

	fd_set readfds;
	FD_ZERO(&readfds);
	int fdmax = udp_fd < tcp_fd ? tcp_fd : udp_fd;
	int ret = 0;

	char * buffer[BUFFSIZE];
	char * out[BUFFSIZE];
	sin_size = sizeof(ext_addr);
	bool cont;

	////////////////////////////////////////////////////////////////////////
	// Loop maintains connection until it receives a "POST /shutdown" request.
	while (true){
		cont = true;
		cache_t c = create_cache(MAXMEM,NULL);
		printf("server: waiting for a connection...\n");
		while(cont){

			FD_SET(tcp_fd, &readfds);
			FD_SET(udp_fd, &readfds);
			if ( (ret = select(fdmax+1, &readfds, NULL, NULL, NULL)) == -1)
			{
			  printf("Select Error.\n");
			  exit(1);
			}
			/*
			if(FD_ISSET(udp_fd,&readfds))
			{
				memset(buffer, '\0', sizeof(buffer));
				if ((rec_len =recvfrom(newfd, buffer, BUFFSIZE-1, 0,(struct sockaddr_in*)&ext_addr,&sin_size)) == -1){
					printf("receive error\n");
					close(newfd);
					return 1;
				}
				inet_ntop(ext_addr.ss_family, &(((struct sockaddr_in*)&ext_addr)->sin_addr), s, sizeof s);
				printf("server: got connection from %s\n", s);

				buffer[rec_len] = '\0';
				printf("Recieved request: %s\n",buffer);
				strcpy(out,process_request(&c,buffer,rec_len));// need a better way of doing this lol
				printf("Sending response: %s\n",out);

		        if (send(newfd, out, strlen(out), 0) == -1){
					printf("send error\n");
		            close(newfd);
		            return 1;
			        }

				if(strcmp(out,"204 No Content: Shutting down")==0){//shutdown
			        if (send(newfd, "Closing connection!\n", 20, 0) == -1){
			            printf("send error\n");
			            close(newfd);
			            return 1;
			        }
					cont = false;
				}
				close(newfd);
			}
			*/
			if(FD_ISSET(tcp_fd, &readfds))
			{
				if ((newfd = accept(tcp_fd, (struct sockaddr *)&ext_addr, &sin_size)) == -1){
					printf("accept error\n");
					return 1;
				}
				inet_ntop(ext_addr.ss_family, &(((struct sockaddr_in*)&ext_addr)->sin_addr), s, sizeof s);
				printf("server: got connection from %s\n", s);

				memset(buffer, '\0', sizeof(buffer));
				if ((rec_len =recv(newfd, buffer, BUFFSIZE-1, 0)) == -1){
					printf("receive error\n");
					close(newfd);
					return 1;
				}
				buffer[rec_len] = '\0';
				printf("Recieved request: %s\n",buffer);
				strcpy(out,process_request(&c,buffer,rec_len));// need a better way of doing this lol
				printf("Sending response: %s\n",out);

		        if (send(newfd, out, strlen(out), 0) == -1){
					printf("send error\n");
		            close(newfd);
		            return 1;
			        }

				if(strcmp(out,"204 No Content: Shutting down")==0){//shutdown
			        if (send(newfd, "Closing connection!\n", 20, 0) == -1){
			            printf("send error\n");
			            close(newfd);
			            return 1;
			        }
					cont = false;
				}
				close(newfd);
			}
			FD_ZERO(&readfds);
		}
	}
	return 0;
}