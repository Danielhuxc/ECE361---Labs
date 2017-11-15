#include"clientContainers.h"

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int connect_to(char* hostname, char* port) {
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    int sockfd;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

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
    // loop through all the results and connect to the first we can
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure

    return sockfd;
}

bool CMDInvalid(int code) {
    if (code == 0) {//neither 
        if (logged || sessJoined) {
            printf("State: logged || sessJoined\n");
            return true;
        }
    } else if (code == 1) {//logged in - not in session
        if (!logged || sessJoined) {
            printf("State: Not logged || sessJoined\n");
            return true;
        }
    } else if (code == 2) {//both
        if (!logged || !sessJoined) {
            printf("State: Not logged || Not sessJoined\n");
            return true;
        }
    } else if (code == 3) {//only care if logged in
        if (!logged) {
            printf("State: Not logged\n");
            return true;
        }
    }
    return false;
}

void joinSess(int sockfd, char* sessid) {
    int nbytes;
    struct msg messageIn, messageOut;
    messageOut.type = JOIN;
    strcpy(messageOut.source, loggedID);
    strcpy(messageOut.data, sessid);
    messageOut.size = strlen(messageOut.data);

    if ((nbytes = send(sockfd, &messageOut, sizeof (struct msg), 0)) == -1) {
        perror("send");
        exit(1);
    }
    if ((nbytes = recv(sockfd, &messageIn, sizeof (struct msg), 0)) == -1) {
        perror("recv");
        exit(1);
    }
    if(messageIn.type==JN_ACK){
        sessJoined=true;
        strcpy(sessID,sessid);
        printf("SESS: Joined %s\n",sessID);
        return;
    }
    else if(messageIn.type=JN_NAK){
        printf("SESS: Join sess failed: %s\n",messageIn.data);
        return;
    }
}