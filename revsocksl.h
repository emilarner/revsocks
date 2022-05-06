#ifndef REVSOCKSL_H
#define REVSOCKSL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <errno.h>

#include "uthash.h"

#include "networkmisc.h"
#include "definitions.h"
#include "config.h"

struct domainres
{
    char domain[256]; // key
    in_addr_t ip; // value
    UT_hash_handle hh; // <--- makes the structure hashable. 
};

struct RevSocks
{
    struct domainres *dnsoverride; 

    bool reverse;

    bool password_auth;
    char username[16];
    char password[32];

    uint16_t port;

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


RevSocks *init_socks5_server(char *username, char *password, uint16_t port, char *domain_file);
int parse_domain_file(RevSocks *rs, char *filename);
void *socks5_client_handler(void *info);
int host_socks5_server(RevSocks *rs);
int host_rev_socks5_server(RevSocks *rs, char *remote_host, int remote_port);

void free_socks5_server(RevSocks *rs);



#endif
