/*
Client
 */
#include"clientContainers.h"

/*
 * 
 */


int main(int argc, char** argv) {
    int nbytes, i;
    int sockfd;
    logged = false;
    sessJoined = false;
    fd_set master, read_fds;
    int fdmax;
    FD_ZERO(&master);

    FD_SET(0, &master);
    fdmax = 0;

    input = malloc(sizeof (char)*LEN);

    for (;;) {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == 0) {//take care of input
                    size_t sz;
                    getline(&input, &sz, stdin);
                    sscanf(input, "%s", cmd);
                    if (strcmp(cmd, "/login") == 0) {
                        if (CMDInvalid(0))
                            break;
                        char id[LEN];
                        char psw[LEN];
                        char hostname[LEN];
                        char port[LEN];
                        struct msg messageOut, messageIn;
                        sscanf(input, "%s %s %s %s %s", cmd, id, psw, hostname, port);
                        if ((sockfd = connect_to(hostname, port)) < 0) {
                            fprintf(stderr, "connection issue\n");
                            break;
                        }
                        messageOut.type = LOGIN;
                        sprintf(messageOut.source, "%s", id);
                        sprintf(messageOut.data, "%s %s", id, psw);
                        messageOut.size = strlen(messageOut.data);

                        if ((nbytes = send(sockfd, &messageOut, sizeof (struct msg), 0)) == -1) {
                            perror("send");
                            exit(1);
                        }
                        if ((nbytes = recv(sockfd, &messageIn, sizeof (struct msg), 0)) == -1) {
                            perror("recv");
                            exit(1);
                        }
                        if (messageIn.type == LO_ACK) {
                            printf("Logged in as %s\n", id);
                            logged = true;
                            strcpy(loggedID, id);
                            FD_SET(sockfd, &master); // add to master set
                            if (sockfd > fdmax) { // keep track of the max
                                fdmax = sockfd;
                            }
                            break;
                        } else if (messageIn.type == LO_NAK) {
                            printf("CONNECTION: Failed to login: %s\n", messageIn.data);
                            close(sockfd);
                            break;
                        }

                    } else if (strcmp(cmd, "/logout") == 0) {
                        if (CMDInvalid(3))
                            break;
                        struct msg messageOut;
                        messageOut.type = EXIT;
                        if ((nbytes = send(sockfd, &messageOut, sizeof (struct msg), 0)) == -1) {
                            perror("send");
                            exit(1);
                        }
                        logged = false;
                        memset(loggedID, 0, LEN);
                        sessJoined = false;
                        memset(sessID, 0, LEN);
                        FD_CLR(sockfd, &master);
                        close(sockfd);
                        printf("LOGOUT: Logged out\n");
                        break;
                    } else if (strcmp(cmd, "/joinsession") == 0) {
                        if (CMDInvalid(1))
                            break;
                        char id[LEN];
                        sscanf(input, "%s %s", cmd, id);
                        joinSess(sockfd, id);
                        break;
                    } else if (strcmp(cmd, "/leavesession") == 0) {
                        if (CMDInvalid(2))
                            break;
                        struct msg messageOut;
                        messageOut.type = LEAVE_SESS;
                        strcpy(messageOut.source, loggedID);
                        if ((nbytes = send(sockfd, &messageOut, sizeof (struct msg), 0)) == -1) {
                            perror("send");
                            exit(1);
                        }
                        printf("SESS: Leaving session %s\n", sessID);
                        sessJoined = false;
                        memset(sessID, 0, LEN);
                        break;
                    } else if (strcmp(cmd, "/createsession") == 0) {
                        if (CMDInvalid(1))
                            break;
                        struct msg messageOut, messageIn;
                        char id[LEN];
                        messageOut.type = NEW_SESS;
                        sscanf(input, "%s %s", cmd, id);
                        sprintf(messageOut.data, "%s", id);
                        strcpy(messageOut.source, loggedID);
                        messageOut.size = strlen(messageOut.data);
                        if ((nbytes = send(sockfd, &messageOut, sizeof (struct msg), 0)) == -1) {
                            perror("send");
                            exit(1);
                        }
                        if ((nbytes = recv(sockfd, &messageIn, sizeof (struct msg), 0)) == -1) {
                            perror("recv");
                            exit(1);
                        }
                        if (messageIn.type == NS_ACK) {
                            joinSess(sockfd, id);
                        } else if (messageIn.type == NS_NAK) {
                            int z;
                            for (z = 0; z < messageIn.size; z++) {
                                printf("%c", messageIn.data[z]);
                            }
                            printf("\n");
                        }
                        break;
                    } else if (strcmp(cmd, "/list") == 0) {
                        if (CMDInvalid(3))
                            break;
                        struct msg messageOut;
                        messageOut.type = QUERY;
                        strcpy(messageOut.source, loggedID);
                        if ((nbytes = send(sockfd, &messageOut, sizeof (struct msg), 0)) == -1) {
                            perror("send");
                            exit(1);
                        }
                        break;
                    } else if (strcmp(cmd, "/quit") == 0) {
                        free(input);
                        return 0;
                    } else if (strcmp(cmd, "/pm") == 0) {
                        if (CMDInvalid(3))
                            break;
                        char id[LEN];
                        char message[LEN];
                        sscanf(input, "%s %s %[^\n]%*c", cmd, id, message);
                        struct msg messageOut;
                        messageOut.type = PM;
                        sprintf(messageOut.data,"%s ",id);
                        strcat(messageOut.data,message);
                        strcpy(messageOut.source, loggedID);
                        messageOut.size = strlen(messageOut.data);
                        if ((nbytes = send(sockfd, &messageOut, sizeof (struct msg), 0)) == -1) {
                            perror("send");
                            exit(1);
                        }
                        break;
                    } else {//message
                        if (CMDInvalid(2))
                            break;
                        struct msg messageOut;
                        messageOut.type = MESSAGE;
                        strcpy(messageOut.source, loggedID);
                        strcpy(messageOut.data, input);
                        messageOut.size = strlen(messageOut.data);
                        if ((nbytes = send(sockfd, &messageOut, sizeof (struct msg), 0)) == -1) {
                            perror("send");
                            exit(1);
                        }
                        break;
                    }
                    break;
                } else {// handle data from server
                    struct msg messageIn;
                    if ((nbytes = recv(i, &messageIn, sizeof (struct msg), 0)) <= 0) {
                        // got error or connection closed by server
                        if (nbytes == 0) {
                            // connection closed
                            printf("Connection: server offline\n");
                        } else {
                            perror("recv");
                        }
                        printf("CONNECTION: Clearing login status\n");
                        logged = false;
                        memset(loggedID, 0, LEN);
                        sessJoined = false;
                        memset(sessID, 0, LEN);
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {//message from server
                        int z;
                        if (messageIn.type == QU_ACK) {
                            for (z = 0; z < messageIn.size; z++) {
                                printf("%c", messageIn.data[z]);
                            }
                            printf("\n");
                        } else if (messageIn.type == MESSAGE) {
                            printf("(");
                            for (z = 0; z < strlen(messageIn.source); z++) {
                                printf("%c", messageIn.source[z]);
                            }
                            printf("): ");
                            for (z = 0; z < messageIn.size; z++) {
                                printf("%c", messageIn.data[z]);
                            }
                            printf("\n");
                        } else if (messageIn.type == PM) {
                            printf("PM: (");
                            for (z = 0; z < strlen(messageIn.source); z++) {
                                printf("%c", messageIn.source[z]);
                            }
                            printf("): ");
                            for (z = 0; z < messageIn.size; z++) {
                                printf("%c", messageIn.data[z]);
                            }
                            printf("\n");
                        }
                    }
                    break;
                }
            }
        }
    }
    free(input);
    return 0;
}