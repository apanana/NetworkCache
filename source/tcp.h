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

int setup_tcp(char * TCPPORT);
void tcp_request(int newfd, cache_t *c);