/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: huxiaoc2
 *
 * Created on September 21, 2017, 10:37 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 1255

void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}
// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, new_fd, numbytes; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    if (argc != 2) {
        fprintf(stderr, "usage: host port#\n");
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof (int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    printf("server: waiting for connections...\n");

    while (1) { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *) &their_addr),
                s, sizeof s);
        printf("server: got connection from %s\n", s);
        if (!fork()) { // this is the child process
            char buf[MAXDATASIZE];
            close(sockfd); // child doesn't need the listener
            if ((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1) {
                perror("recv");
                exit(1);
            }
            buf[numbytes] = '\0';
            if (strcmp(buf, "ftp") == 0) {
                if (send(new_fd, "yes", 3, 0) == -1)
                    perror("send");
            } else {
                if (send(new_fd, "no", 2, 0) == -1)
                    perror("send");
            }

            //receiver first packet
            if ((numbytes = recv(new_fd, buf, MAXDATASIZE, 0)) == -1) {
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
            
            if (send(new_fd, "ACK", 3, 0) == -1)
                perror("send");

            for (int i = 0; i < total-1; i++) {
                if ((numbytes = recv(new_fd, buf, MAXDATASIZE, 0)) == -1) {
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

                if (send(new_fd, "ACK", 3, 0) == -1)
                    perror("send");
            }

            fclose(fp);



            close(new_fd);
            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }
    return 0;
}