#include <stdbool.h>
#include <time.h>
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

int udp_server_setup(char * UDPPORT);
void udp_server_request(int newfd, cache_t *c);

int connect_udp_server(cache_t cache);
char * udp_client_request(int sockfd, char* buf,cache_t cache);