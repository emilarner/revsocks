#include "revsocksserver.h"

RevSocksServer *init_revsocksserver(int remote_port, int local_port)
{
    /* Ports cannot be above 65535. */
    if (remote_port > 65535 || local_port > 65536)
        return NULL;

    /* Ports below 1 do not make sense. Yes, 0 is non-bindable/non-usable. */
    if (remote_port < 1 || local_port < 1)
        return NULL; 

    RevSocksServer *srv = (RevSocksServer*) malloc(sizeof(RevSocksServer));

    srv->control_fd = 0;
    srv->local_fd = 0;
    srv->remote_fd = 0;

    srv->control_port = remote_port + 1;
    srv->remote_port = remote_port;
    srv->local_port = local_port;

    utarray_new(srv->available_fds, &ut_int_icd);
    srv->echo = false;

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

    if (listen(srv->fd, LISTEN_BACKLOG) < 0)
        return errno;

    return 0;
}


void *local_server(void *information)
{
    #ifdef UNIX
        signal(SIGPIPE, SIG_IGN); // signal() does not exist on Windows.
    #endif

    struct ServerInformation *info = (struct ServerInformation*) information;

    if (info->srv->echo)
        printf("Started local server on port %d\n", ntohs(info->sock.sin_port));

    while (true)
    {
        struct sockaddr_in client;
        socklen_t length = sizeof(client);

        int cfd = 0;

        /* Accept the local connection; print any errors. */
        if ((cfd = accept(info->fd, (struct sockaddr*) &client, &length)) < 0)
        {
            if (info->srv->echo)
                fprintf(stderr, "accept() failed in local_server: %s\n", strerror(errno));

            csleep(SECOND);
            continue;
        }

        /* If there is no alive control connection, close the connection. */
        /* Then, reiterate. */
        if (!info->srv->control_cfd)
        {
            sclose(cfd);
            continue;
        }

        uint8_t connect_msg = REVSOCKS_CONNECT;

        /* Send the message that we need a connection for the client we just received. */
        if (rsend(info->srv->control_cfd, &connect_msg, 1) < 0)
        {
            if (info->srv->echo)
            {
                fprintf(stderr, "Critical error: control server error!: %s\n", strerror(errno));
                fprintf(stderr, "~~~~~~~~~~~~~~~~^ FD: %d\n", info->srv->control_cfd);
            }

            /* We don't have a working control connection, so let's clarify that */
            info->srv->control_cfd = 0;
            continue;
        }

        utarray_push_back(info->srv->available_fds, &cfd);
    }
}

void *glue(void *p)
{
    #ifdef UNIX
        signal(SIGPIPE, SIG_IGN);
    #endif

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

        if (status == -1)
            break;

        /* When local has data, send it to the remote end. */
        if (FD_ISSET(pair->local, &two))
        {
            ssize_t len = recv(pair->local, buffer, sizeof(buffer), MSG_DONTWAIT);
            
            /* Closed. */
            if (len <= 0)
                break;

            if (send(pair->remote, buffer, len, MSG_DONTWAIT) <= 0)
                break;
        }

        /* When remote has data, send it to the local end. */
        if (FD_ISSET(pair->remote, &two))
        {
            ssize_t len = recv(pair->remote, buffer, sizeof(buffer), MSG_DONTWAIT);
            
            /* Closed. */
            if (len <= 0)
                break;

            if (send(pair->local, buffer, len, MSG_DONTWAIT) <= 0)
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
    #ifdef UNIX
        signal(SIGPIPE, SIG_IGN);
    #endif
    
    struct ServerInformation *info = (struct ServerInformation*) information;

    if (info->srv->echo)
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
            if (info->srv->echo)
                fprintf(stderr, "accept() failed in remote server: %s\n", strerror(errno));
            
            csleep(ERROR_SLEEP);
            continue;
        }

        /* We need to know their purposes of connecting. */
        /* Are they going to be used for controlling the SOCKS5 reverse server? */
        /* Or are they going to be connecting upon a CONNECT request, to bypass the firewall? */ 
        uint8_t handshake;
        rrecv(cfd, &handshake, 1);

        
        switch (handshake)
        {
            /* They want to serve as a control receiver. */
            case REVSOCKS_CONTROL:
            {
                if (info->srv->echo)
                    printf("Control connection accepted. You are now able to use them as a SOCKS5 proxy.");
                
                info->srv->control_cfd = cfd;
                break;
            }
            
            /* They're just here for data exchange via SOCKS5 proxy. */
            case REVSOCKS_NORMAL:
            {
                /* This must transcend the stack. malloc(), here we go! */
                struct FDPair *fds = (struct FDPair*) malloc(sizeof(struct FDPair));
                fds->remote = cfd;
                fds->local = *((int*) utarray_back(info->srv->available_fds));
                utarray_pop_back(info->srv->available_fds);

                #ifdef UNIX
                    pthread_t gluet;
                    pthread_create(&gluet, NULL, glue, (void*) fds);
                #endif

                #ifdef WINDOWS
                    HANDLE gluet = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) glue, 
                            (void*) fds, 0, NULL);
                #endif

                break;
            }
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
    if (setsockopt(local.fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        return errno;

    if (setsockopt(remote.fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        return errno;

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

    utarray_free(srv->available_fds);
    free(srv);
}
