#ifndef NETWORKMISC_H 
#define NETWORKMISC_H

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>


int reliable_recv(int fd, void *dest, size_t length);




#endif