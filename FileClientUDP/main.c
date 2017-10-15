/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: Administrator
 *
 * Created on September 27, 2017, 2:08 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define MAXDATASIZE 1000

int main(int argc, char *argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    char buf[MAXDATASIZE];
    int rv;
    int numbytes;
    if (argc != 3) {
        fprintf(stderr, "usage: client hostname port#\n");
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "failed to create socket\n");
        return 2;
    }

    /*
    if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
            p->ai_addr, p->ai_addrlen)) == -1) {
        perror("sendto");
        exit(1);
    }
     */

    char input[MAXDATASIZE];
    char filename[MAXDATASIZE];
    printf("Intput (ftp filename):");
    scanf("%s %s", input, filename);

    if (strcmp(input, "ftp") != 0) {
        printf("invalid argument");
        return 0;
    }

    struct timeval start, end;

    gettimeofday(&start, 0);

    if (access(filename, F_OK) != -1) {
        if ((numbytes = sendto(sockfd, "ftp", 3, 0,
                p->ai_addr, p->ai_addrlen)) == -1) {
            perror("send");
            exit(1);
        }
    } else {
        printf("file %s not found, exiting\n", filename);
        freeaddrinfo(servinfo);
        close(sockfd);
        return 0;
    }

    if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
            (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    gettimeofday(&end, 0);
    printf("round trip time was %10ld ms\n", end.tv_usec - start.tv_usec);

    buf[numbytes] = '\0';

    if (strcmp(buf, "yes") == 0) {
        printf("A file transfer can start\n");
    } else {
        freeaddrinfo(servinfo);
        close(sockfd);
        return 0;
    }

    FILE *fp;
    fp = fopen(filename, "rb");

    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    rewind(fp);
    int Parts = (size / MAXDATASIZE) + 1;

    //making ack buffer
    char ack[4];
    fd_set readfds;
    FD_SET(sockfd, &readfds);
    struct timeval timeout;
    timeout.tv_sec = 5;

    for (int i = 0; i < Parts; i++) {
        //reset ack
        bzero(ack, 4);
        int sizeOfThisPart;
        if (i + 1 != Parts)
            sizeOfThisPart = MAXDATASIZE;
        else
            sizeOfThisPart = size - i*MAXDATASIZE;
        char header[255];
        sprintf(header, "%d:%d:%d:%s:\0", Parts, i + 1, sizeOfThisPart, filename);
        char buffer[sizeOfThisPart + strlen(header)];
        char* data = buffer + strlen(header);
        strcpy(buffer, header);
        fread(data, 1, sizeOfThisPart, fp);

        //take this out in Sec 4
        if (sendto(sockfd, buffer, sizeof (buffer), 0,
                p->ai_addr, p->ai_addrlen) == -1)
            perror("send");
        /*Sec 4

        while (strcmp(ack,"ACK")!=0) {
            if (sendto(sockfd, buffer, sizeof (buffer), 0,
                    p->ai_addr, p->ai_addrlen) == -1)
                perror("send");

            if (rv = select(sockfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
                if ((numbytes = recvfrom(sockfd, ack, 4, 0,
                        (struct sockaddr *) &their_addr, &addr_len)) == -1) {
                    perror("recv");
                    exit(1);
                }
            }

            //timeout
            if (rv == 0)
                printf("timeout\n");
        }
         */

    }

    freeaddrinfo(servinfo);
    close(sockfd);
    return 0;
}
