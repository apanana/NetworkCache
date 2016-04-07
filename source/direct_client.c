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

#define BUFFSIZE 100 // max number of bytes we can get at once

// A function that allows us to take user inputs from command line
// up to an "Enter" keystroke. Found on:
// http://stackoverflow.com/questions/314401/how-to-read-a-line-from-the-console-in-c
// By Johannes Schaub
char * get_line(void) {
    char * line = malloc(100), * linep = line;
    size_t lenmax = 100, len = lenmax;
    int c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;

        if(--len == 0) {
            len = lenmax;
            char * linen = realloc(linep, lenmax *= 2);

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}

char * PORT = "3490";
char * HOST = NULL;

int main(int argc, char *argv[])
{
    ////////////////////////////////////////////////////////////////////////
    // PARSING COMMAND LINE INPUTS
    // Get maxmem and portnum and host from command line
    char * port = NULL;
    char * host = NULL;
    int opt;
    // Using getopt to parse the command line arguments
    while ((opt = getopt (argc, argv, "ph")) != -1){
        switch(opt){
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case '?':
                if (optopt == 'h')
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

    // Updating our MAXMEM, PORT and HOST based on command line inputs
    if (port == NULL){
        printf("Using default port number\n");
    }
    else PORT = port;
    if (host == NULL){
        printf("Using localhost\n");
        printf("Initial settings:portnum = %s, host = localhost\n",PORT);
    }
    else {
        HOST = host;
        printf("Initial settings:portnum = %s, host = %s\n",PORT);
    }

    ////////////////////////////////////////////////////////////////////////
    // SETTING UP OUR CLIENT
    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    if ((rv = getaddrinfo(HOST, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
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
    freeaddrinfo(servinfo); // all done with this structure

    ////////////////////////////////////////////////////////////////////////
    // Loop maintains connection until client sends a POST /shutdown" request.
    char * buffer[BUFFSIZE];
    int rec_len;

    char* buf = get_line();
    printf("BUFFER: %s\n",buf);
    if ((numbytes = send(sockfd, buf, BUFFSIZE-1, 0)) == -1) {
        perror("send");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("Sent to server: %s\n",buf);

    memset(buffer, '\0', sizeof(buffer));
    if ((rec_len =recv(sockfd, buffer, BUFFSIZE-1, 0)) == -1){
        printf("receive error\n");
        close(sockfd);
        return 1;
    }
    buffer[rec_len] = '\0';
    printf("Recieved response: %s\n",buffer);

    free(buf);

    close(sockfd);
    return 0; 
}



