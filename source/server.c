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

#include "tcp.h"
#include "udp.h"

#define BACKLOG 10
#define BUFFSIZE 100

int main(int argc, char *argv[]){
	char * TCP = "3490";
	char * UDP = "4590";
	int MAXMEM = 1000;
	////////////////////////////////////////////////////////////////////////
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
				exit(1);
		  	default:
				abort();
		}
	}
	printf("Initial settings: maxmem = %d, TCP = %s, UDP = %s\n",MAXMEM,TCP,UDP);

	////////////////////////////////////////////////////////////////////////
	// SETTING UP OUR SOCKETS	
	int tcp_fd = setup_tcp(TCP);
	int udp_fd = setup_udp(UDP);

	fd_set readfds;
	FD_ZERO(&readfds);
	int fdmax = udp_fd < tcp_fd ? tcp_fd : udp_fd;
	int ret = 0;

    ////////////////////////////////////////////////////////////////////////
	// Getting server ready for a connection
	int newfd;
	struct sockaddr_storage ext_addr;
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];
	sin_size = sizeof(ext_addr);

	cache_t c = create_cache(MAXMEM,NULL);

	////////////////////////////////////////////////////////////////////////
	// Loop maintains connection until it receives a "POST /shutdown" request.
	while (true){
		printf("Server: Waiting for a connection...\n");
		FD_SET(tcp_fd, &readfds);
		FD_SET(udp_fd, &readfds);
		if ( (ret = select(fdmax+1, &readfds, NULL, NULL, NULL)) == -1)
		{
		  printf("Select Error\n");
		  exit(1);
		}
		if(FD_ISSET(udp_fd,&readfds))
		{	
			udp_request(udp_fd,&c);
		}
		if(FD_ISSET(tcp_fd, &readfds))
		{
			if ((newfd = accept(tcp_fd, (struct sockaddr *)&ext_addr, &sin_size)) == -1){
				printf("Accept Error\n");
				exit(1);
			}
			inet_ntop(ext_addr.ss_family, &(((struct sockaddr_in*)&ext_addr)->sin_addr), s, sizeof s);
			printf("Server: Got connection from %s\n", s);
			tcp_request(newfd,&c);
			close(newfd);
		}
	}
	return 0;
}