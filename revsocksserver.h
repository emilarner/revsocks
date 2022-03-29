#ifndef REVSOCKSSERVER_H
#define REVSOCKSSERVER_H

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

#include "stack.h"
#include "config.h"

struct RevSocksServer
{
    Stack *stack;

    int remote_fd;
    int control_fd;
    int control_cfd;
    int local_fd;
    
    uint16_t remote_port;
    uint16_t control_port;
    uint16_t local_port;
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


RevSocksServer *init_revsocksserver(int remote_port, int local_port);
int start_revsocksserver(RevSocksServer *srv);
void free_revsocksserver(RevSocksServer *srv);


void *control_server(void *information);
void *local_server(void *information);
void *remote_server(void *information);


#endif