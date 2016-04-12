#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
// Parsing command line inputs
#include <unistd.h>
#include <errno.h>
// Network Libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include "cache.h"
#include "tcp.h"
#include "udp.h"

char * TCPPORT = "3490";
char * UDPPORT = "4590";
char * HOST = NULL;

struct cache_obj{
    char * host;
    char * tcpport;
    char * udpport;
    struct addrinfo * tcpservinfo;
    struct addrinfo * udpservinfo;
};

char * val_from_json(char * json){
    strtok(json," ");
    strtok(NULL," ");
    strtok(NULL," ");
    char * val = strtok(NULL," ");
    char * out = calloc(strlen(val)-1,1);
    strncpy(out,val,strlen(val)-2);
    return out;
}

uint64_t size_from_head(char * head){
    strtok(head,"\n");
    strtok(NULL,"\n");
    strtok(NULL,"\n");
    strtok(NULL,"\n");
    strtok(NULL,"\n");
    strtok(NULL," ");
    // I dont expect the size to be something crazy huge, 1E30 is pretty darn big.
    char * val[30];
    strcpy(val,strtok(NULL,"\n"));
    uint64_t out = strtoll(val,(char **)NULL,10);
    return out;
}

////////////////////////////////////////////////////////////////////////
// CACHE FUNCTIONS
//
cache_t create_cache(uint64_t maxmem, hash_func hash){
    cache_t c = calloc(1,sizeof(struct cache_obj));
    extern char * TCPPORT;
    extern char * HOST;
    extern char * UDPPORT;
    c->host = HOST;
    c->tcpport = TCPPORT;
    c->udpport = UDPPORT;
    c->tcpservinfo = calloc(1,sizeof(struct addrinfo));
    c->udpservinfo = calloc(1,sizeof(struct addrinfo));

    // Getting address info and setting it into c->tcpservinfo
    struct addrinfo hints;
    int status;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    if ((status = getaddrinfo(c->host, c->tcpport, &hints, &c->tcpservinfo)) != 0) {
        free(c->tcpservinfo);
        free(c);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    if ((status = getaddrinfo(c->host, c->udpport, &hints, &c->udpservinfo)) != 0) {
        free(c->udpservinfo);
        free(c);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    //connecting to server and sending a request to POST /memsize/maxmem
    int sockfd = connect_tcp_server(c);
    char * request[50];
    memset(&request,'\0',sizeof(request));
    sprintf(request,"POST /memsize/%u\n",maxmem);
    tcp_client_request(sockfd,request);
    close(sockfd);
    return c;
}

void cache_set(cache_t cache, key_type key, val_type val, uint32_t val_size){
    int sockfd = connect_tcp_server(cache);
    int req_len = 10 + strlen(key) + strlen(key);
    char * request[req_len];
    memset(&request,'\0',sizeof(request));
    sprintf(request,"PUT /%s/%s\n",key,val);
    tcp_client_request(sockfd,request);
    close(sockfd);
}

val_type cache_get(cache_t cache, key_type key, uint32_t *val_size){
    int sockfd = connect_udp_server(cache);
    // int sockfd = connect_tcp_server(cache);
    int req_len = 10 + strlen(key);
    char *request = calloc(req_len,10);
    sprintf(request,"GET /%s\n",key);
    char * response = udp_client_request(sockfd,request,cache);
    // char * response = tcp_client_request(sockfd,request);

    if (strcmp(response,"404 Not Found!\n")==0){
        return NULL;
    }
    val_type v = val_from_json(response);
    *val_size = strlen(v);
    free(request);
    close(sockfd);
    return v;
}

void cache_delete(cache_t cache, key_type key){
    int sockfd = connect_tcp_server(cache);
    int request_len = 10 + strlen(key);
    char * request[request_len];
    memset(&request,'\0',sizeof(request));
    sprintf(request,"DELETE /%s\n",key);
    tcp_client_request(sockfd,request);
    close(sockfd);
}

uint64_t cache_space_used(cache_t cache){
    int sockfd = connect_tcp_server(cache);
    char * response = tcp_client_request(sockfd,"HEAD /k\n");
    close(sockfd);
    uint64_t out = size_from_head(response);
    return out;
}

void destroy_cache(cache_t cache){
    int sockfd = connect_tcp_server(cache);
    tcp_client_request(sockfd,"POST /shutdown\n");
    close(sockfd);
}