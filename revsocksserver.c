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


void *local_server(void *information)
{
    struct ServerInformation *info = (struct ServerInformation*) information;

    printf("Started local server on port %d\n", ntohs(info->sock.sin_port));

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
            csleep(SECOND);
            continue;
        }

        /* Send the message that we need a connection for the client we just received. */
        if (rsend(info->srv->control_cfd, "CONNECT", sizeof("CONNECT")) < 0)
        {
            fprintf(stderr, "Critical error: control server error!: %s\n", strerror(errno));
            fprintf(stderr, "~~~~~~~~~~~~~~~~^ FD: %d\n", info->srv->control_cfd);
            continue;
        }

        /* Evil! Dereferencing this 'pointer' will cause a crash -- it's just an fd!*/
        stack_push(info->srv->stack, (void*) (long) cfd);
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

    char buffer[EXCHANGE_BUFFER_SIZE];

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

    return NULL; // to make MSVC happy
}

void *remote_server(void *information)
{
    struct ServerInformation *info = (struct ServerInformation*) information;

    printf("Started remote server on port %d\n", ntohs(info->sock.sin_port));

    /* Again, if you forget to set the socklen_t variable to the size of the sockaddr_in structure, may you suffer! */
    /* PROBABLY one of the easiest things to forget -- so don't do it. */
    struct sockaddr_in client;
    socklen_t length = sizeof(client);

    while (true)
    {
        int cfd = 0;

        if ((cfd = accept(info->fd, (struct sockaddr*) &client, &length)) < 0)
        {
            fprintf(stderr, "accept() failed in remote server: %s\n", strerror(errno));
            csleep(SECOND);
            continue;
        }

        char handshake[16];
        rrecv(cfd, handshake, sizeof(handshake));

        //recv(cfd, handshake, sizeof(handshake), 0);

        /* This connection is a control connection; they be commanded. */
        if (!memcmp(handshake, "CONTROL", sizeof("CONTROL")))
        {
            printf("Control connection accepted. FD: %d\n", cfd);
            info->srv->control_cfd = cfd;
        }
        else
        {
            /* This must transcend the stack. malloc(), here we go! */
            struct FDPair *fds = (struct FDPair*) malloc(sizeof(struct FDPair));
            fds->remote = cfd;
            fds->local = (long) stack_pop(info->srv->stack);

#ifdef UNIX
            pthread_t tmp;
            pthread_create(&tmp, NULL, glue, (void*) fds);
#endif

#ifdef WINDOWS
            HANDLE gluet = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) glue, (void*) fds, 0, NULL);
#endif
        }
    }
}

int start_revsocksserver(RevSocksServer *srv)
{
    struct ServerInformation local;
    local.srv = srv;

    struct ServerInformation remote;
    remote.srv = srv;
    

    int status = 0;

    if ((status = create_server(&remote, srv->remote_port)) < 0)
        return status;

    if ((status = create_server(&local, srv->local_port)) < 0)
        return status;

    
    /* Again, the skidded code which works so well. */ 
    setsockopt(local.fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    setsockopt(remote.fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

#ifdef UNIX
    /* START POSIX only */
    pthread_t localt;
    pthread_t remotet;

    pthread_create(&localt, NULL, local_server, (void*) &local);
    pthread_create(&remotet, NULL, remote_server, (void*) &remote);

    /* END POSIX only */
#endif

#ifdef WINDOWS
    HANDLE localt = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) local_server, (void*) &local, 0, NULL);
    HANDLE remotet = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) remote_server, (void*) &remote, 0, NULL);
#endif

    while (true)
        getc(stdin);

    return 0;
}


void free_revsocksserver(RevSocksServer *srv)
{
    sclose(srv->control_fd);
    sclose(srv->local_fd);
    sclose(srv->remote_fd);

    stack_free(srv->stack);
    free(srv);
}
