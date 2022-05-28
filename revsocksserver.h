#ifndef REVSOCKSSERVER_H
#define REVSOCKSSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "utarray.h"

#include "networkmisc.h"
#include "stack.h"
#include "config.h"

struct RevSocksServer
{
    UT_array *available_fds;

    int remote_fd;
    int control_fd;
    int control_cfd;
    int local_fd;
    
    uint16_t remote_port;
    uint16_t control_port;
    uint16_t local_port;

    bool echo;
};

typedef struct RevSocksServer RevSocksServer;

struct ServerInformation
{
    RevSocksServer *srv;
    int fd;
    struct sockaddr_in sock;
};

int create_server(struct ServerInformation *srv, int port);


struct FDPair
{
    int remote;
    int local;
};

void *glue(void *p);

/* Make a RevSocksServer and return a pointer. NULL is returned on error. */
RevSocksServer *init_revsocksserver(int remote_port, int local_port);

/* 
Start the server. Pass the RevSocksServer pointer in.
If there is any error, the corresponding errno is returned from this function.
This function blocks indefinitely.
*/
int start_revsocksserver(RevSocksServer *srv);

/* Cleanup resources allocated to the RevSocksServer object. */ 
void free_revsocksserver(RevSocksServer *srv);


void *local_server(void *information);
void *remote_server(void *information);


#endif