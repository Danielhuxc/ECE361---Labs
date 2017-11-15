/*
Server
 */

#include "container.h"

/*
 * 
 */


in_port_t get_in_port(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*) sa)->sin_port);
    }

    return (((struct sockaddr_in6*) sa)->sin6_port);
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}
// get sockaddr, IPv4 or IPv6:

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: server port#\n");
        exit(1);
    }
    char PORT[10];
    strcpy(PORT, argv[1]);
    fd_set master; // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    int fdmax; // maximum file descriptor number
    int listener; // listening socket descriptor
    int newfd; // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[256]; // buffer for client data
    int nbytes;
    char remoteIP[INET6_ADDRSTRLEN];
    int remoteport;
    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;
    struct addrinfo hints, *ai, *p;
    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "CONNECTION: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }
    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai); // all done with this
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    // add the listener to the master set
    FD_SET(listener, &master);
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    //initialize user data
    init_usr();
    root_Session = NULL;
    online = NULL;
    // main loop
    for (;;) {
        read_fds = master; // copy it
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        // run through the existing connections looking for data to read
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                            (struct sockaddr *) &remoteaddr,
                            &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) { // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                                "socket %d\n",
                                inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*) &remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                                newfd);
                        remoteport = ntohs(get_in_port((struct sockaddr *) &remoteaddr));
                    }
                } else {
                    // handle data from a client
                    struct msg messageIn;
                    if ((nbytes = recv(i, &messageIn, sizeof (struct msg), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        deleteUserbysock(i);
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        if (messageIn.type == LOGIN) {
                            struct msg messageOut;
                            char usr[LEN];
                            char psw[LEN];
                            sscanf(messageIn.data, "%s %s", usr, psw);
                            int res = login(usr, psw, i);
                            if (res == 0) {
                                printf("LOGIN: User (%s) logged in.\n", &usr);
                                messageOut.type = LO_ACK;
                                findOnlinebyname(usr)->port = remoteport;
                                strcpy(findOnlinebyname(usr)->s, remoteIP);
                            } else if (res == -1) {
                                printf("LOGIN: User (%s) is already online\n", usr);
                                messageOut.type = LO_NAK;
                                sprintf(messageOut.data, "LOGIN: User (%s) is already online\n", usr);
                                messageOut.size = strlen(messageOut.data);
                            } else if (res == -2) {
                                printf("LOGIN: No matching username and password pair found\n");
                                messageOut.type = LO_NAK;
                                sprintf(messageOut.data, "LOGIN: No matching username and password pair found\n");
                                messageOut.size = strlen(messageOut.data);
                            }
                            if ((nbytes = send(i, &messageOut, sizeof (struct msg), 0)) == -1) {
                                perror("send");
                                exit(1);
                            }
                        } else if (messageIn.type == EXIT) {
                            deleteUserbysock(i);
                            close(i);
                            FD_CLR(i, &master);
                        } else if (messageIn.type == JOIN) {
                            struct sess* sessPtr = findSession(messageIn.data);
                            struct msg messageOut;
                            if (!sessPtr) {//no such session
                                messageOut.type = JN_NAK;
                                sprintf(messageOut.data, "Session (%s) does not exist", messageIn.data);
                                messageOut.size = strlen(messageOut.data);
                                if ((nbytes = send(i, &messageOut, sizeof (struct msg), 0)) == -1) {
                                    perror("send");
                                    exit(1);
                                }
                                printf("SESS: User (%s) tried to join (%s) but session (%s) doesn't exist\n", messageIn.source, messageIn.data, messageIn.data);
                                continue;
                            }
                            messageOut.type = JN_ACK;
                            strcpy(messageOut.data, messageIn.data);
                            messageOut.size = strlen(messageOut.data);
                            if ((nbytes = send(i, &messageOut, sizeof (struct msg), 0)) == -1) {
                                perror("send");
                                exit(1);
                            }
                            sessPtr->users++;
                            FD_SET(i, &sessPtr->fds);
                            strcpy(findOnlinebysock(i)->sessId, sessPtr->id);
                            printf("SESS: User (%s) joined session (%s)\n", messageIn.source, sessPtr->id);
                        } else if (messageIn.type == LEAVE_SESS) {
                            deleteUserfromSession(messageIn.source);
                            break;
                        } else if (messageIn.type == NEW_SESS) {
                            struct sess* sessPtr = findSession(messageIn.data);
                            if (sessPtr) {
                                struct msg messageOut;
                                messageOut.type = NS_NAK;
                                sprintf(messageOut.data, "SESS: Session with name %s is already taken", messageIn.data);
                                messageOut.size = strlen(messageOut.data);
                                if ((nbytes = send(i, &messageOut, sizeof (struct msg), 0)) == -1) {
                                    perror("send");
                                    exit(1);
                                }
                                continue;
                            } else {
                                struct msg messageOut;
                                messageOut.type = NS_ACK;
                                if ((nbytes = send(i, &messageOut, sizeof (struct msg), 0)) == -1) {
                                    perror("send");
                                    exit(1);
                                }
                                newSession(messageIn.data);
                                continue;
                            }
                        } else if (messageIn.type == MESSAGE) {
                            for (j = 0; j <= fdmax; j++) {
                                if (j != i && j != listener) {
                                    if (FD_ISSET(j, &findSession(findOnlinebyname(messageIn.source)->sessId)->fds)) {
                                        if ((nbytes = send(j, &messageIn, sizeof (struct msg), 0)) == -1) {
                                            perror("send");
                                            exit(1);
                                        }
                                    }
                                }
                            }
                            continue;
                        } else if (messageIn.type == QUERY) {
                            struct msg messageOut;
                            messageOut.type = QU_ACK;
                            sprintf(messageOut.data, "Online sessions:\n");
                            appendSessions(messageOut.data);
                            strcat(messageOut.data, "\nOnline Users:\n");
                            appendUsers(messageOut.data);
                            messageOut.size = strlen(messageOut.data);
                            if ((nbytes = send(i, &messageOut, sizeof (struct msg), 0)) == -1) {
                                perror("send");
                                exit(1);
                            }
                            continue;
                        } else if (messageIn.type == PM) {
                            char to[LEN];
                            char message[LEN];
                            sscanf(messageIn.data, "%s %[^\n]%*c", to, message);
                            struct msg messageOut;
                            int dest;
                            struct onlineUser* ptr = findOnlinebyname(to);
                            if (ptr == NULL) {
                                messageOut.type = MESSAGE;
                                sprintf(messageOut.data, "Personal Message failed: User %s is not online", to);
                                strcpy(messageOut.source, "Server");
                                messageOut.size = strlen(messageOut.data);
                                dest = findOnlinebyname(messageIn.source)->sock;
                            } else {//user is online
                                messageOut.type = PM;
                                strcpy(messageOut.data,message);
                                strcpy(messageOut.source, messageIn.source);
                                messageOut.size = strlen(messageOut.data);
                                dest = findOnlinebyname(to)->sock;
                            }
                            if ((nbytes = send(dest, &messageOut, sizeof (struct msg), 0)) == -1) {
                                perror("send");
                                exit(1);
                            }
                            continue;
                        } else {
                            printf("Invalid message type");
                            continue;
                        }
                        /*
                                                // we got some data from a client
                                                for (j = 0; j <= fdmax; j++) {
                                                    // send to everyone!
                                                    if (FD_ISSET(j, &master)) {
                                                        // except the listener and ourselves
                                                        if (j != listener && j != i) {
                                                            if (send(j, buf, nbytes, 0) == -1) {
                                                                perror("send");
                                                            }
                                                        }
                                                    }
                                                }
                         */

                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

    return 0;
}
