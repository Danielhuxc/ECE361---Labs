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


struct msg {
    unsigned int type;
    unsigned int size;
    unsigned char source[LEN];
    unsigned char data[LEN];
};

struct onlineUser {
    char id[LEN];
    struct onlineUser* next;
    struct onlineUser* last;
    char sessId[LEN];
    char s[46];//INET6_ADDRSTRLEN
    int port;
    int sock;
    char invited[LEN];
};

struct sess {
    char id[LEN];
    fd_set fds;
    int users;
    struct sess* last;
    struct sess* next;
};

struct userInfo {
    char username[LEN];
    char psw[LEN];
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

struct sess* root_Session;
struct onlineUser* online;
struct userInfo users[10];
void init_usr();
int login(char* name,char* psw,int sock);//-1 on already online, -2 name or psw doesn't match
bool auth(char* name,char* psw);
void addToOnline(char* name, int sock);
struct onlineUser* findOnlinebysock(int sock);
struct onlineUser* findOnlinebyname(char* name);
void deleteUserbysock(int sock);
void deleteUserbyname(char* name);

struct sess* findSession(char* id);
void newSession(char* id);
void deleteSession(char* id);

void deleteUserfromSession(char* userID);

void appendUsers(char* string);
void appendSessions(char* string);