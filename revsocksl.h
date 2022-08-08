#ifndef REVSOCKSL_H
#define REVSOCKSL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <errno.h>

#include "networkmisc.h"
#include "definitions.h"
#include "config.h"


struct RevSocks
{
    bool reverse;

    bool password_auth;
    char username[16];
    char password[32];

    uint16_t port;

    int fd;
    struct sockaddr_in sock;

    /* If false, do not print anything out to stdout. */
    bool echo;

    /* Set this to something to do something every time a client connects. */
    void (*connection_callback)(struct RevSocks *s, int fd, struct sockaddr_in *addr);
};

typedef struct RevSocks RevSocks;

struct Client
{
    RevSocks *sock;
    int fd;
    struct sockaddr_in addr;
};

/* If no callback is explicitly given, this will be the default one for when clients connect. */
void default_connection_callback(RevSocks *r, int fd, struct sockaddr_in *addr);

/* 
Initializes the RevSocks object. If no password auth, set username and password to NULL. 
If not using DNS override, set domain_file to NULL.
Returns a pointer to a RevSocks object in heap memory, so you will need to free this later!
*/
RevSocks *init_socks5_server(char *username, char *password, uint16_t port);


int parse_domain_file(RevSocks *rs, char *filename);
void *socks5_client_handler(void *info);

/* 
Host a normal SOCKS5 proxy server on the port chosen in the initialization, along with other settings. 
Blocks indefinitely.
*/
int host_socks5_server(RevSocks *rs);

/* 
Host a reversed SOCKS5 proxy server. You need to connect to a remote host over the firewall.
The remote host must be running the RevSocks reverse server--found in revsocksserver.h/c
remote_host can be an ip address or a domain, since it is resolved using gethostbyname().
Blocks indefinitely.
*/
int host_rev_socks5_server(RevSocks *rs, char *remote_host, int remote_port);

/* Call this upon a RevSocks pointer so that we can free it--or suffer a memory leak! */ 
void free_socks5_server(RevSocks *rs);



#endif
