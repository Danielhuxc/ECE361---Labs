/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: Administrator
 *
 * Created on September 27, 2017, 2:07 AM
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
#define MAXDATASIZE 1255
// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXDATASIZE];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    if (argc != 2) {
        fprintf(stderr, "usage: host port#\n");
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "failed to bind socket\n");
        return 2;
    }
    freeaddrinfo(servinfo);
    printf("waiting to recvfrom...\n");

    while (1) {
        addr_len = sizeof their_addr;


        if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
                (struct sockaddr *) &their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';
        /*
        printf("got packet from %s\n",
                inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *) &their_addr),
                s, sizeof s));
        printf("packet is %d bytes long\n", numbytes);
         */
        if (strcmp(buf, "ftp") == 0) {
            if ((numbytes = sendto(sockfd, "yes", 3, 0,
                    (struct sockaddr *) &their_addr, addr_len)) == -1) {
                perror("send");
                exit(1);
            }
        } else {
            if ((numbytes = sendto(sockfd, "no", 2, 0,
                    (struct sockaddr *) &their_addr, addr_len)) == -1) {
                perror("send");
                exit(1);
            }
        }

        if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
                (struct sockaddr *) &their_addr, &addr_len)) == -1) {
            perror("recv");
            exit(1);
        }
        printf("%d bytes received\n", numbytes);

        int total, frag_no, size;
        int iterator = 0;
        char temp[255];

        for (int i = 0; buf[iterator] != ':'; i++) {
            temp[i] = buf[iterator];
            temp[i + 1] = '\0';
            iterator++;
        }
        total = atoi(temp);
        iterator++;

        for (int i = 0; buf[iterator] != ':'; i++) {
            temp[i] = buf[iterator];
            temp[i + 1] = '\0';
            iterator++;
        }

        frag_no = atoi(temp);
        iterator++;

        for (int i = 0; buf[iterator] != ':'; i++) {
            temp[i] = buf[iterator];
            temp[i + 1] = '\0';
            iterator++;
        }

        size = atoi(temp);
        iterator++;

        for (int i = 0; buf[iterator] != ':'; i++) {
            temp[i] = buf[iterator];
            temp[i + 1] = '\0';
            iterator++;
        }
        iterator++;

        char* file_frag = buf + iterator;

        printf("filename: %s, headerlength: %d, Total:%d,Frag_no:%d,Size:%d\n", temp, iterator, total, frag_no, size);

        FILE *fp;
        fp = fopen(temp, "wb");
        fwrite(file_frag, 1, size, fp);

        if ((numbytes = sendto(sockfd, "ACK", 3, 0,
                    (struct sockaddr *) &their_addr, addr_len)) == -1)
            perror("send");

        for (int i = 0; i < total - 1; i++) {
            if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
                (struct sockaddr *) &their_addr, &addr_len)) == -1) {
                perror("recv");
                exit(1);
            }
            printf("%d bytes received\n", numbytes);

            int iterator = 0;

            for (int i = 0; buf[iterator] != ':'; i++) {
                temp[i] = buf[iterator];
                temp[i + 1] = '\0';
                iterator++;
            }
            total = atoi(temp);
            iterator++;

            for (int i = 0; buf[iterator] != ':'; i++) {
                temp[i] = buf[iterator];
                temp[i + 1] = '\0';
                iterator++;
            }

            frag_no = atoi(temp);
            iterator++;

            for (int i = 0; buf[iterator] != ':'; i++) {
                temp[i] = buf[iterator];
                temp[i + 1] = '\0';
                iterator++;
            }

            size = atoi(temp);
            iterator++;

            for (int i = 0; buf[iterator] != ':'; i++) {
                temp[i] = buf[iterator];
                temp[i + 1] = '\0';
                iterator++;
            }
            iterator++;

            char* file_frag = buf + iterator;

            printf("filename: %s, headerlength: %d, Total:%d,Frag_no:%d,Size:%d\n", temp, iterator, total, frag_no, size);
            fwrite(file_frag, 1, size, fp);

            /*Sec 4
            if ((numbytes = sendto(sockfd, "ACK", 3, 0,
                    (struct sockaddr *) &their_addr, addr_len)) == -1)
                perror("send");
            */
        }

        fclose(fp);
    }
    close(sockfd);
    return 0;
}