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
#include "request.h"
#include "cache.h"

int tcp_server_setup(char * TCPPORT);
void tcp_server_request(int newfd, cache_t *c);

int connect_tcp_server(cache_t cache);
char * tcp_client_request(int sockfd, char* buf);