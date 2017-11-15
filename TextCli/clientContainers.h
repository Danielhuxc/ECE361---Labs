#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

#define LEN 256


bool logged;
bool sessJoined;
char loggedID[LEN];
char sessID[LEN];
char* input;
char cmd[LEN];

struct msg {
    unsigned int type;
    unsigned int size;
    unsigned char source[LEN];
    unsigned char data[LEN];
};

enum Type {
    LOGIN = 0,
    LO_ACK = 1,
    LO_NAK = 2,
    EXIT = 3,
    JOIN = 4,
    JN_ACK = 5,
    JN_NAK = 6,
    LEAVE_SESS = 7,
    NEW_SESS = 8,
    NS_ACK = 9,
    NS_NAK = 10,
    MESSAGE = 11,
    QUERY = 12,
    QU_ACK = 13,
    PM = 14
};

void *get_in_addr(struct sockaddr *sa);
int connect_to(char* hostname, char* port);

bool CMDInvalid(int code);

void joinSess(int sockfd, char* sessid);