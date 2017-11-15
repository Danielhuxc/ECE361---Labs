#include"container.h"

void init_usr() {
    strcpy(users[0].username, "alice");
    strcpy(users[0].psw, "psw123");
    strcpy(users[1].username, "bob");
    strcpy(users[1].psw, "psw456");
    strcpy(users[2].username, "carol");
    strcpy(users[2].psw, "psw789");
    strcpy(users[3].username, "david");
    strcpy(users[3].psw, "psw098");
}

bool auth(char* name, char* psw) {
    int i;
    for (i = 0; i < 4; i++) {
        if (strcmp(users[i].username, name) == 0 && strcmp(users[i].psw, psw) == 0)
            return true;
    }
    return false;
}

void addToOnline(char* name, int sock) {
    if (online == NULL) {
        online = malloc(sizeof (struct onlineUser));
        online->last = NULL;
        online->next = NULL;
        strcpy(online->id, name);
        online->sock = sock;
        return;
    } else {
        struct onlineUser* ptr = online;
        while (ptr->next != NULL) {
            ptr = ptr->next;
        }
        ptr->next = malloc(sizeof (struct onlineUser));
        ptr->next->last = ptr;
        ptr->next->next = NULL;
        strcpy(ptr->next->id, name);
        ptr->next->sock = sock;
        return;
    }
}

int login(char* name, char*psw, int sock) {
    if (findOnlinebyname(name))
        return -1;
    if (auth(name, psw)) {//match found in db
        addToOnline(name, sock);
        return 0;
    }
    return -2;
}

struct onlineUser* findOnlinebysock(int sock) {
    struct onlineUser* ptr;
    for (ptr = online; ptr != NULL; ptr = ptr->next) {
        if (ptr->sock == sock)
            return ptr;
    }
    return NULL;
}

struct onlineUser* findOnlinebyname(char* name) {
    struct onlineUser* ptr;
    for (ptr = online; ptr != NULL; ptr = ptr->next) {
        if (strcmp(ptr->id, name) == 0)
            return ptr;
    }
    return NULL;
}

void deleteUserbysock(int sock) {
    struct onlineUser* ptr = findOnlinebysock(sock);
    if (ptr != NULL) {
        if (strlen(ptr->sessId) != 0)
            deleteUserfromSession(ptr->id);
        if ((ptr->last == NULL) && (ptr->next == NULL)) {//first and last
            online = NULL;
        } else if (ptr->last == NULL) {//first one
            ptr->next->last = NULL;
            online = ptr->next;
        } else if (ptr->next == NULL) {//last one
            ptr->last->next = NULL;
        } else {//in the middle
            ptr->last->next = ptr->next;
            ptr->next->last = ptr->last;
        }
        printf("LOGOUT: User (%s) logged out\n", ptr->id);
        free(ptr);
    }
    return;
}

void deleteUserbyname(char* name) {
    struct onlineUser* ptr = findOnlinebyname(name);
    if (ptr != NULL) {
        if (strlen(ptr->sessId) != 0)
            deleteUserfromSession(name);
        if ((ptr->last == NULL) && (ptr->next == NULL)) {//first and last
            online = NULL;
        } else if (ptr->last == NULL) {//first one
            ptr->next->last = NULL;
            online = ptr->next;
        } else if (ptr->next == NULL) {//last one
            ptr->last->next = NULL;
        } else {//in the middle
            ptr->last->next = ptr->next;
            ptr->next->last = ptr->last;
        }
        free(ptr);
        printf("LOGOUT: User (%s) logged out\n", name);
    }
    return;
}

struct sess* findSession(char* id) {
    struct sess* ptr;
    for (ptr = root_Session; ptr != NULL; ptr = ptr->next) {
        if (strcmp(ptr->id, id) == 0)
            return ptr;
    }
    return NULL;
}

void newSession(char* id) {
    if (findSession(id) != NULL)
        return;
    struct sess* ptr = root_Session;
    if (ptr == NULL) {
        ptr = malloc(sizeof (struct sess));
        strcpy(ptr->id, id);
        ptr->last = NULL;
        ptr->next = NULL;
        FD_ZERO(&ptr->fds);
        ptr->users = 0;
        root_Session = ptr;
        return;
    }
    while (ptr->next != NULL) {
        ptr = ptr->next;
    }
    ptr->next = malloc(sizeof (struct sess));
    ptr->next->next = NULL;
    ptr->next->last = ptr;
    ptr = ptr->next;
    strcpy(ptr->id, id);
    FD_ZERO(&ptr->fds);
    ptr->users = 0;
    return;
}

void deleteSession(char* id) {
    struct sess* ptr = findSession(id);
    if (ptr == NULL)
        return;
    if ((ptr->last == NULL) && (ptr->next == NULL)) {//first and last
        root_Session = NULL;
    } else if (ptr->last == NULL) {//first one
        ptr->next->last=NULL;
        root_Session = ptr->next;
    } else if (ptr->next == NULL) {//last one
        ptr->last->next = NULL;
    } else {//in the middle
        ptr->last->next = ptr->next;
        ptr->next->last = ptr->last;
    }
    free(ptr);
    return;
}

void deleteUserfromSession(char* userID) {
    struct onlineUser* usrPtr = findOnlinebyname(userID);
    if (usrPtr == NULL) {
        printf("ERROR: trying to kick (%s) out of session but user doesn't exist\n", userID);
        return;
    }
    struct sess* sessPtr = findSession(usrPtr->sessId);
    if (sessPtr == NULL) {
        printf("ERROR: trying to kick (%s) out of session but user not in session\n", userID);
        return;
    }
    memset(usrPtr->sessId, 0, LEN);
    FD_CLR(usrPtr->sock, &sessPtr->fds);
    sessPtr->users--;
    printf("SESS: User (%s) kicked from session (%s)\n", usrPtr->id, sessPtr->id);
    if (sessPtr->users == 0) {
        char sessid[LEN];
        strcpy(sessid, sessPtr->id);
        deleteSession(sessid);
        printf("SESS: Session (%s) deleted for lack of users\n", sessid);
    }
    return;
}

void appendUsers(char* string) {
    struct onlineUser* ptr;
    int count = 0;
    for (ptr = online; ptr != NULL; ptr = ptr->next) {
        strcat(string, ptr->id);
        if (count % 3 == 2)
            strcat(string, "\n");
        else
            strcat(string, " ");
        count++;
    }
    return;
}

void appendSessions(char* string) {
    struct sess* ptr;
    int count = 0;
    for (ptr = root_Session; ptr != NULL; ptr = ptr->next) {
        strcat(string, ptr->id);
        if (count % 3 == 2)
            strcat(string, "\n");
        else
            strcat(string, " ");
        count++;
    }
    return;
}