/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: huxiaoc2
 *
 * Created on September 21, 2017, 10:55 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdbool.h>
#define MAXDATASIZE 1000 // max number of bytes we can get at once
#define PACKETSIZE 1500
// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    if (argc != 3) {
        fprintf(stderr, "usage: client hostname port#\n");
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;

        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure
    char input[MAXDATASIZE];
    char filename[MAXDATASIZE];
    printf("Intput (ftp <file name):");
    scanf("%s <%s", input, filename);

    int finish = 0;
    for (int i = 0; i < MAXDATASIZE && finish != 1; i++) {
        if (filename[i] == '>' && i != 0) {
            filename[i] = '\0';
            finish = 1;
        }
    }

    struct timeval start, end;

    gettimeofday(&start, 0);

    if (access(filename, F_OK) != -1) {
        if (send(sockfd, "ftp", 3, 0) == -1)
            perror("send");
    } else {
        printf("file %s not found, exiting\n", filename);
        close(sockfd);
        return 0;
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    gettimeofday(&end, 0);

    printf("round trip time was %10ld ms\n", end.tv_usec - start.tv_usec);
    buf[numbytes] = '\0';
    if (strcmp(buf, "yes") == 0) {
        printf("A file transfer can start\n");
    } else {
        close(sockfd);
        return 0;
    }
    /*
    char* buffer[PACKETSIZE];
    
    FILE *fp;
    *fp = FILE *fopen(filename,"rb");
    
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    int Parts = (size/1000) +1;
    
    for(int i=0; i< Parts;i++){
        int sizeOfThisPart = 0;
        if(i+1!=Parts)
            sizeOfThisPart=1000;
        else
            sizeOfThisPart=size-i*1000;
        fprintf("%d:%d:%d:%s:",Parts,i+1,sizeOfThisPart,filename);
        
    }
    
    
    */
    
    
    
    
    
    
    
    
    close(sockfd);
    return 0;
}