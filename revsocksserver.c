#include "revsocksserver.h"

RevSocksServer *init_revsocksserver(int remote_port, int local_port)
{
    RevSocksServer *srv = (RevSocksServer*) malloc(sizeof(RevSocksServer));

    srv->control_fd = 0;
    srv->local_fd = 0;
    srv->remote_fd = 0;

    srv->control_port = remote_port + 1;
    srv->remote_port = remote_port;
    srv->local_port = local_port;

    srv->stack = stack_init();

    return srv;
}

int create_server(struct ServerInformation *srv, int port)
{
    if ((srv->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return errno;

    srv->sock.sin_port = htons(port);
    srv->sock.sin_family = AF_INET;
    srv->sock.sin_addr.s_addr = INADDR_ANY;

    if ((bind(srv->fd, (struct sockaddr*) &srv->sock, (socklen_t) sizeof(struct sockaddr_in))) < 0)
        return errno;

    listen(srv->fd, LISTEN_BACKLOG);

    return 0;
}

void *control_server(void *information)
{
    printf("Started control server.\n");

    struct ServerInformation *info = (struct ServerInformation*) information;

    struct sockaddr_in client;
    socklen_t length = sizeof(client);

    if ((info->srv->control_cfd = accept(info->fd, (struct sockaddr*) &client, &length)) < 0)
    {
        fprintf(stderr, "Error accepting control server connection: %s\n", strerror(errno));
        return NULL; 
    }

    printf("Accepted control server connection. FD: %d\n", info->srv->control_cfd);

    while (true)
        getc(stdin);
}

void *local_server(void *information)
{
    printf("Started local server.\n");

    struct ServerInformation *info = (struct ServerInformation*) information;

    printf("File descriptor: %d\n", info->fd);

    while (true)
    {
        struct sockaddr_in client;
        socklen_t length = sizeof(client);

        int cfd = 0;

        /* Accept the local connection; print any errors. */
        if ((cfd = accept(info->fd, (struct sockaddr*) &client, &length)) < 0)
        {
            fprintf(stderr, "accept() failed in local_server: %s\n", strerror(errno));
            usleep(1000000);
            continue;
        }

        if (send(info->srv->control_cfd, "CONNECT", sizeof("CONNECT"), 0) < 0)
        {
            fprintf(stderr, "Critical error: control server error!: %s\n", strerror(errno));
            fprintf(stderr, "~~~~~~~~~~~~~~~~^ FD: %d\n", info->srv->control_cfd);
            continue;
        }

        /* Evil! Dereferencing this 'pointer' will cause a crash -- it's just an fd!*/
        stack_push(info->srv->stack, (void*) cfd);
    }
}

void *glue(void *p)
{
    struct FDPair *pair = (struct FDPair*) p;

    /* Timeout for select() */
    struct timeval timeout;
    timeout.tv_usec = 0;
    timeout.tv_sec = SELECT_TIMEOUT;

    fd_set two;

    char buffer[4096];

    while (true)
    {
        FD_SET(pair->local, &two);
        FD_SET(pair->remote, &two);

        int status = select(MAX_SELECT_FDS, &two, NULL, NULL, &timeout);

        /* When local has data, send it to the remote end. */
        if (FD_ISSET(pair->local, &two))
        {
            size_t len = recv(pair->local, buffer, sizeof(buffer), MSG_DONTWAIT);
            
            /* Closed. */
            if (!len)
                break;

            if (send(pair->remote, buffer, len, MSG_DONTWAIT) < 0)
                break;
        }

        /* When remote has data, send it to the local end. */
        if (FD_ISSET(pair->remote, &two))
        {
            size_t len = recv(pair->remote, buffer, sizeof(buffer), MSG_DONTWAIT);
            
            /* Closed. */
            if (!len)
                break;

            if (send(pair->local, buffer, len, MSG_DONTWAIT) < 0)
                break;
        }
    }

    close(pair->local);
    close(pair->remote);
    free(pair);
}

void *remote_server(void *information)
{
    printf("Started remote server.\n");

    struct ServerInformation *info = (struct ServerInformation*) information;

    struct sockaddr_in client;
    socklen_t length = sizeof(client);

    while (true)
    {
        int cfd = 0;

        if ((cfd = accept(info->fd, (struct sockaddr*) &client, &length)) < 0)
        {
            fprintf(stderr, "accept() failed in remote server: %s\n", strerror(errno));
            usleep(1000000);
            continue;
        }

        /* This must transcend the stack. malloc(), here we go! */
        struct FDPair *fds = (struct FDPair*) malloc(sizeof(struct FDPair));
        fds->remote = cfd;
        fds->local = (int) stack_pop(info->srv->stack);

        pthread_t tmp;
        pthread_create(&tmp, NULL, glue, (void*) fds);
    }
}

int start_revsocksserver(RevSocksServer *srv)
{
    struct ServerInformation control;
    control.srv = srv;

    struct ServerInformation local;
    local.srv = srv;

    struct ServerInformation remote;
    remote.srv = srv;
    

    int status = 0;

    if ((status = create_server(&control, srv->control_port)) < 0)
        return status;

    if ((status = create_server(&remote, srv->remote_port)) < 0)
        return status;

    if ((status = create_server(&local, srv->local_port)) < 0)
        return status;

    
    /* START POSIX only */
    pthread_t controlt;
    pthread_t localt;
    pthread_t remotet;

    pthread_create(&controlt, NULL, control_server, (void*) &control);
    pthread_create(&localt, NULL, local_server, (void*) &local);
    pthread_create(&remotet, NULL, remote_server, (void*) &remote);

    /* END POSIX only */

    while (true)
        getc(stdin);

    return 0;
}


void free_revsocksserver(RevSocksServer *srv)
{
    close(srv->control_fd);
    close(srv->local_fd);
    close(srv->remote_fd);

    stack_free(srv->stack);
    free(srv);
}
