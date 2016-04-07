#include "cache.h"
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

#define BUFFSIZE 1000 // max number of bytes we can get at once

char * PORT = "3490";
char * HOST = NULL;

struct cache_obj{
    char * host;
    char * port;
    struct addrinfo * servinfo;
};

int connect_server(cache_t cache){
    int sockfd;
    struct addrinfo *p;
    char s[INET6_ADDRSTRLEN];
    for(p = cache->servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue; 
        }
    break; }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family, &(((struct sockaddr_in*)&p->ai_addr)->sin_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    return sockfd;
}

char * send_rec(int sockfd, char* buf){
    int numbytes;
    if ((numbytes = send(sockfd, buf, strlen(buf), 0)) == -1) {
        perror("");
        exit(1);
    }
    char * buffer[BUFFSIZE];
    int rec_len;
    memset(&buffer, '\0', sizeof(buffer));
    if ((rec_len =recv(sockfd, buffer, BUFFSIZE-1, 0)) == -1){
        printf("receive error\n");
        close(sockfd);
        exit(1);
    }
    buffer[rec_len] = '\0';
    printf("Recieved response: %s\n",buffer);
    return buffer;
}

char * val_from_json(char * json){
    strtok(json," ");
    strtok(NULL," ");
    strtok(NULL," ");
    char * val[BUFFSIZE];
    strcpy(val,strtok(NULL,"{"));
    char* out[strlen(val)];
    memset(&out,'\0',sizeof(out));
    strncpy(out,val,strlen(val)-1);
    return out;
}

uint64_t size_from_head(char * head){
    strtok(head,"\n");
    strtok(NULL,"\n");
    strtok(NULL,"\n");
    strtok(NULL,"\n");
    strtok(NULL,"\n");
    strtok(NULL," ");
    char * val[BUFFSIZE];
    strcpy(val,strtok(NULL,"\n"));
    uint64_t out = strtoll(val,(char **)NULL,10);
    return out;
}


cache_t create_cache(uint64_t maxmem, hash_func hash){
    cache_t c = calloc(1,sizeof(struct cache_obj));
    extern char * PORT;
    extern char * HOST;
    c->host = HOST;
    c->port = PORT;
    c->servinfo = calloc(1,sizeof(struct addrinfo));
    // Getting address info and setting it into c->servinfo
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    int status;
    if ((status = getaddrinfo(c->host, c->port, &hints, &c->servinfo)) != 0) {
        free(c->servinfo);
        free(c);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }
    //connecting to server and sending a request to POST /memsize/maxmem
    int sockfd = connect_server(c);
    char* buf[BUFFSIZE];
    memset(&buf,'\0',sizeof(buf));
    sprintf(&buf,"POST /memsize/%u\n",maxmem);
    char * buffer[BUFFSIZE];
    strcpy(buffer,send_rec(sockfd,buf));
    close(sockfd);
    printf("end %p\n",c);
    return c;
}

void cache_set(cache_t cache, key_type key, val_type val, uint32_t val_size){
    int sockfd = connect_server(cache);
    char* buf[BUFFSIZE];
    memset(&buf,'\0',sizeof(buf));
    sprintf(buf,"PUT /%s/%s\n",key,val);
    printf("BUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUFBUF\n%s\n",buf);
    char * buffer[BUFFSIZE];
    strcpy(buffer,send_rec(sockfd,buf));

    close(sockfd);
}

val_type cache_get(cache_t cache, key_type key, uint32_t *val_size){
    int sockfd = connect_server(cache);
    char* buf[BUFFSIZE];
    memset(&buf,'\0',sizeof(buf));
    sprintf(buf,"GET /%s\n",key);
    char * buffer[BUFFSIZE];
    strcpy(buffer,send_rec(sockfd,buf));

    close(sockfd);
    if (strcmp(buffer,"404 Not Found!\n")==0){
        return NULL;
    }
    val_type v = val_from_json(buffer);
    *val_size = strlen(v);
    return v;
}

void cache_delete(cache_t cache, key_type key){
    int sockfd = connect_server(cache);
    char* buf[BUFFSIZE];
    memset(&buf,'\0',sizeof(buf));
    sprintf(buf,"DELETE /%s\n",key);
    char * buffer[BUFFSIZE];
    strcpy(buffer,send_rec(sockfd,buf));

    close(sockfd);
}

uint64_t cache_space_used(cache_t cache){
    int sockfd = connect_server(cache);
    char * buffer[BUFFSIZE];
    strcpy(buffer,send_rec(sockfd,"HEAD /k\n"));
    close(sockfd);
    uint64_t out = size_from_head(buffer);
    return out;
}

void destroy_cache(cache_t cache){
    int sockfd = connect_server(cache);
    char * buffer[BUFFSIZE];
    strcpy(buffer,send_rec(sockfd,"POST /shutdown\n"));
    close(sockfd);
}