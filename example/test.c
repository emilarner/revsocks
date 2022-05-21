#include <stdio.h>
#include <stdlib.h>

#include "../revsocksl.h"

void print_client(RevSocks *r, int fd, struct sockaddr_in *addr)
{
    printf("An innocent client connected: %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
}

int main(int argc, char **argv, char **envp)
{
    RevSocks *rs = init_socks5_server(NULL, NULL, 8080, NULL);
    rs->connection_callback = &print_client;
    host_socks5_server(rs);

    return 0;
}