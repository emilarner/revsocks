#ifndef REVSOCKSL_H
#define REVSOCKSL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pthread.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "definitions.h"
#include "config.h"

struct RevSocks
{
    in_addr_t server_ip;

    bool reverse;

    bool password_auth;
    char username[16];
    char password[32];

    uint16_t port;
    uint16_t big_port;

    int fd;
    struct sockaddr_in sock;
};

typedef struct RevSocks RevSocks;

struct Client
{
    RevSocks *sock;
    int fd;
    struct sockaddr_in addr;
};

void reliable_recv(int fd, void *dest, size_t length);

RevSocks *init_socks5_server(char *username, char *password, uint16_t port);
void *socks5_client_handler(void *info);
int host_socks5_server(RevSocks *rs);
int host_rev_socks5_server(RevSocks *rs, char *remote_host, int remote_port);

void free_socks5_server(RevSocks *rs);



#endif
