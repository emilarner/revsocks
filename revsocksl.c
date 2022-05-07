#include "revsocksl.h"

RevSocks *init_socks5_server(char *username, char *password, uint16_t port, char *domain_file)
{
    RevSocks *r = (RevSocks*) malloc(sizeof(RevSocks));

    /* If these are NULL, no password authentication */
    if (username == NULL || password == NULL)
    {
        r->password_auth = false; 
    }
    else 
    {
        r->password_auth = true;
        strncpy(r->username, username, sizeof(r->username));
        strncpy(r->password, password, sizeof(r->password));
    }


    r->port = port;
    r->fd = 0;

    /* Initializing to zero... just to make sure... */ 
    memset(&r->sock, 0, sizeof(struct sockaddr_in));

    if (domain_file != NULL)
    {
        if (parse_domain_file(r, domain_file) < 0)
        {
            fprintf(stderr, "Revsocks initialization failed!\n");
            return NULL;
        }
    }

    return r;
}

int parse_domain_file(RevSocks *rs, char *filename)
{
    FILE *fp = fopen(filename, "r");
    
    char line[512];

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        /* Ignore comments. */
        if (line[0] == '#')
            continue;

        /* Ignore whitespace */
        if (line[0] == ' ' || line[0] == '\n' || line[0] == '\r')
            continue;

        /* Get rid of newline character. */
        if (line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0'; 


        /* Detect syntatical errors in the domain override file. */ 
        if (strchr(line, '=') == NULL)
        {
            fprintf(stderr, "DNS override wrong file format... missing '=' delimiter.\n");
            return -1;
        }

        struct domainres *pair = (struct domainres*) malloc(sizeof(struct domainres));

        char *domain = strtok(line, "=");
        strncpy(pair->domain, domain, sizeof(pair->domain));
        
        char *ip = strtok(NULL, "=");

        printf("DNS Domain: %s\n", pair->domain);

        struct hostent *ent = gethostbyname(ip);
        if (ent == NULL)
        {
            #ifdef UNIX
                fprintf(stderr, "DNS override error: IP/DOMAIN %s cannot be resolved: %s\n", 
                    ip, hstrerror(h_errno));
            #endif

            #ifdef WINDOWS
                fprintf(stderr, "DNS override error: IP/DOMAIN %s cannot be resolved; WSA CODE: %d\n", 
                    ip, WSAGetLastError()); 
            #endif 
            continue;
        }

        pair->ip = ((struct in_addr*) ent->h_addr_list[0])->s_addr;

        HASH_ADD_STR(rs->dnsoverride, domain, pair);
        memset(line, 0, sizeof(line)); 
    }

    fclose(fp);
}

/* where the SOCKS5 protocol is implemented (to a certain degree.) */
void *socks5_client_handler(void *info)
{
    /* SIGPIPE is the bane of our existence--disable it. */
    #ifdef UNIX
        signal(SIGPIPE, SIG_IGN);
    #endif

    struct Client *client = (struct Client*) info;

    /* Get the methods that the client wants. */
    struct ClientSelectMethod cmethod;
    if (rrecv(client->fd, &cmethod, sizeof(cmethod)) == -1)
        goto cleanup; 


    /* Get all of the available, the number of which specified by the header sent by the client above. */
    uint8_t methods[256];
    if (rrecv(client->fd, methods, cmethod.nomethods) == -1)
        goto cleanup;
    
    struct ServerMethodSelection mymethod;
    mymethod.version = SOCKS5;
    mymethod.selection = client->sock->password_auth ? UsernamePassword : NoAuthentication; 

    if (rsend(client->fd, &mymethod, sizeof(mymethod)) == -1)
        goto cleanup;

    
    /* Username-Password sub-negotiation, given we are in that mode. */
    if (client->sock->password_auth)
    {
        struct ClientUserAuthentication userauth;
        if (rrecv(client->fd, &userauth, sizeof(userauth)) == -1)
            goto cleanup;


        /* Allocate data for the username and password they are going to send us. */
        char username[257];
        char password[257];

        if (rrecv(client->fd, username, userauth.username_len) == -1)
            goto cleanup;

        uint8_t password_len = 0;

        if (rrecv(client->fd, &password_len, 1) == -1)
            goto cleanup;

        if (rrecv(client->fd, password, password_len) == -1)
            goto cleanup;

        /* Give them null terminators so they play nicely with C's string functions. */
        username[userauth.username_len] = '\0';
        password[password_len] = '\0';

        struct ServerAuthenticationResponse authresp;
        authresp.version = 1;
        authresp.status = 0;

        if (!!strcmp(username, client->sock->username) || !!strcmp(password, client->sock->password))
        {
            authresp.status = 1;
        }

        if (rsend(client->fd, &authresp, sizeof(authresp)) == -1)
            goto cleanup;

        if (authresp.status != 0)
            goto cleanup;
    }
   

    /* Get what the client wants. */
    struct ClientRequest clientreq;
    if (rrecv(client->fd, &clientreq, sizeof(clientreq)) == -1)
        goto cleanup;

    /* This will be the response to the client. */
    struct ServerResponse response;
    response.reply = Success;
    response.reserved = 0;
    response.version = SOCKS5;

    /* Don't care about IPv6 */
    if (clientreq.address_type == IPV6)
    {
        response.reply = AddressTypeNotSupported;
    }

    /* Have not (and will not) implemented anything other than CONNECT. */
    if (clientreq.command != Connect)
    {
        response.reply = CommandNotSupported;
    }

    
    struct sockaddr_in destaddr;
    destaddr.sin_family = AF_INET;

    char domain[257];
    size_t dlength = 0;

    /* Depending on the address type specified by the client, use different methods to obtain it. */
    switch (clientreq.address_type)
    {
        /* IPv4 is the easiest: just read the next 4 bytes! */
        case IPV4:
        {
            in_addr_t ipaddr = 0;
            if (rrecv(client->fd, &ipaddr, sizeof(ipaddr)) == -1)
                goto cleanup;

            destaddr.sin_addr.s_addr = ipaddr;
            response.address_type = IPV4;
            break;
        }

        /* The first byte will contain the length of the domain. Then, read that length. */
        /* But! Domain resolution functions want a null-terminated string, so make sure there */
        /* is enough space for a null character. The SOCKS5 protocol explicitly states that this domain */
        /* text is NOT null-terminated. */
        case DOMAIN:
        {
            if (rrecv(client->fd, &dlength, 1) == -1)
                goto cleanup;

            if (rrecv(client->fd, &domain, dlength) == -1)
                goto cleanup;

            /* Give a null terminator at the end of the string. */
            domain[dlength] = '\0';

            struct domainres *override = NULL;
            HASH_FIND_STR(client->sock->dnsoverride, domain, override);

            response.address_type = DOMAIN;

            if (override != NULL)
                destaddr.sin_addr.s_addr = override->ip;
            else
            {
                /* Statically allocated; no need to free. */
                struct hostent *resolution = gethostbyname(domain);


                /* Domain resolution failed. */
                if (!resolution)
                {
                    response.reply = HostUnreachable;
                    break;
                }

    

                /* struct assignment is more efficient than using memcpy(). */
                destaddr.sin_addr = *((struct in_addr*) resolution->h_addr_list[0]);
            }

            break;
        }
    }

    /* Get the port. */
    if (rrecv(client->fd, &destaddr.sin_port, 2) == -1)
        goto cleanup;

    /* Connect to the remote end that the client wants to connect to, through us. */
    /* If there is an error with connecting to it, report that to the client. */
    int end = socket(AF_INET, SOCK_STREAM, 0);
    if ((connect(end, (struct sockaddr*) &destaddr, (socklen_t) sizeof(destaddr))) < 0)
    {
        switch (errno)
        {
            case ECONNREFUSED:
            {
                response.reply = ConnectionRefused;
                break;
            }

            case ENETUNREACH:
            {
                response.reply = NetworkUnreachable;
                break;
            }
            
            default:
            {
                response.reply = Failure;
                break;
            }
        }
    }



    /* Send the response. */
    if (rsend(client->fd, &response, sizeof(response)) == -1)
        goto cleanup;
    
    /* Depending on the address type, send it in the respective format. */
    switch (response.address_type)
    {
        case IPV4:
        {
            if (rsend(client->fd, &destaddr.sin_addr.s_addr, 4) == -1)
                goto cleanup;

            break;
        }

        case DOMAIN:
        {
            if (rsend(client->fd, &dlength, 1) == -1)
                goto cleanup;

            if (rsend(client->fd, &domain, dlength) == -1)
                goto cleanup;

            break;
        }
    }

    /* Port must now be sent. */
    if (rsend(client->fd, &destaddr.sin_port, 2) == -1)
        goto cleanup;

    if (response.reply != Success)
        goto cleanup; 

    /* Get select ready, so that we can glue the connections together. */
    fd_set exchange;
    struct timeval timeout;
    timeout.tv_usec = 0;
    timeout.tv_sec = SELECT_TIMEOUT;
    

/* For windows, set non-blocking, as MSG_DONTWAIT is not implemented for recv() on Windows. */
#ifdef WINDOWS
    int blocking = 1;
    ioctlsocket(client->fd, FIONBIO, &blocking);
    ioctlsocket(end, FIONBIO, &blocking);
#endif

    while (true)
    {
        char buffer[EXCHANGE_BUFFER_SIZE];

        /* select() will overwrite it each time, so we must reset it each time. */
        FD_SET(client->fd, &exchange);
        FD_SET(end, &exchange);

        int status = select(MAX_SELECT_FDS, &exchange, NULL, NULL, &timeout);

        /* Important to detect if the other end has closed. We do this by checking for -1 on send() */
        /* recv() will return 0 when the socket closes, so check for that too. */
        /* If the client has data, send it to the other end. */
        if (FD_ISSET(client->fd, &exchange))
        {
            size_t len = 0;
            if ((len = recv(client->fd, buffer, sizeof(buffer), MSG_DONTWAIT)) == 0)
                break;

            if (send(end, buffer, len, MSG_DONTWAIT) == -1)
                break;

        }

        /* And vice versa. */
        if (FD_ISSET(end, &exchange))
        {
            size_t len = 0;
            
            if ((len = recv(end, buffer, sizeof(buffer), MSG_DONTWAIT)) == 0)
                break;
            
            if (send(client->fd, buffer, len, MSG_DONTWAIT) == -1)
                break;
        }
    }


    /* Close the client connection and deallocate memory. */
    cleanup:
        sclose(client->fd);
        sclose(end);
        free(client);
}

int host_socks5_server(RevSocks *rs)
{
#ifdef UNIX
    signal(SIGPIPE, SIG_IGN);
#endif

    /* Try to create the socket. */
    if ((rs->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Cannot create socket server: %s\n", strerror(errno));
        return -1;
    }

    /* Neat trick skidded off of StackOverFlow! */
    /* Prevents 'Address is already in use' annoying errors when debugging after a crash. */
    setsockopt(rs->fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    /* Setup socket connection information. */
    rs->sock.sin_addr.s_addr = INADDR_ANY;
    rs->sock.sin_port = htons(rs->port);
    rs->sock.sin_family = AF_INET;

    /* Attempt to bind on that port. */
    if ((bind(rs->fd, (struct sockaddr*) &rs->sock, (socklen_t) sizeof(struct sockaddr_in))) < 0)
    {
        fprintf(stderr, "Failed to bind on port %d: %s\n", rs->port, strerror(errno));
        return -2;
    }

    listen(rs->fd, LISTEN_BACKLOG);
    
    while (true)
    {
        struct sockaddr_in client;

        /* If you forget to set it to the size of the structure, may you suffer! */
        socklen_t client_len = sizeof(client);
        
        int cfd = 0;

        if ((cfd = accept(rs->fd, (struct sockaddr*) &client, &client_len)) < 0)
        {
            fprintf(stderr, "Failed to accept on SOCKS5 server!: %s\n", strerror(errno));
            continue;
        }

        printf("Connection from: %s:%d !\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        /* Allocate a heap struct containing necessary information. */
        struct Client *c = (struct Client*) malloc(sizeof(struct Client));
        c->fd = cfd;
        c->addr = client;
        c->sock = rs;

#ifdef UNIX
        /* Just throw it into a separate thread. NOT using epoll()... */
        pthread_t tmp;
        pthread_create(&tmp, NULL, socks5_client_handler, (void*) c);
#endif

#ifdef WINDOWS
        HANDLE tmpthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) socks5_client_handler, (void*)c, 0, NULL);
#endif
    }

}

/* Host a reversed SOCKS5 server. We won't be binding. Instead, we connect to a remote host. */
int host_rev_socks5_server(RevSocks *rs, char *remote_host, int remote_port)
{
    #ifdef UNIX
        signal(SIGPIPE, SIG_IGN);
    #endif

    /* Why do people hate gethostbyname()? getaddrinfo() is a nightmare. */
    struct hostent *resolution = gethostbyname(remote_host);

    if (resolution == NULL)
    {
        #ifdef UNIX
            fprintf(stderr, "Error in remote hostname resolution: %s\n", hstrerror(h_errno));
        #endif
        
        #ifdef WINDOWS
            fprintf(stderr, "Ah... Windows. Don't want to get string of error... Here's the code: %d\n",
                WSAGetLastError()
            );
        #endif
        
        return -1;
    }

    int control_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in control;

    struct in_addr address = *((struct in_addr*) resolution->h_addr_list[0]);

    control.sin_addr = address;
    control.sin_port = htons(remote_port);
    control.sin_family = AF_INET;


    if (connect(control_fd, (struct sockaddr*) &control, (socklen_t) sizeof(control)) < 0)
    {
        fprintf(stderr, "Error connecting to control: %s\n", strerror(errno));
        return -1;
    }

    /* Declare that our first connection be that of a control socket. */
    /* It shall tell us when to connect to the remote end as a normal socket, */
    /* upon every SOCKS5 proxy request on the local end of the reverse server. */

    uint8_t handshake_msg = REVSOCKS_CONTROL;
    rsend(control_fd, &handshake_msg, 1); 

    while (true)
    {
        uint8_t connect_msg = 0;
        ssize_t len = rrecv(control_fd, &connect_msg, 1);

        if (len <= 0)
        {
            fprintf(stderr, "Control server has errored or died.\n");
            break;
        }

        /* Look for the CONNECT message. Inspired by firehop! */
        if (connect_msg != REVSOCKS_CONNECT)
        {
            fprintf(stderr, "Error: non-connect msg sent to control socket.\n");
            fprintf(stderr, "Here's what was sent: %d\n", connect_msg);

            csleep(SECOND);
            continue;
        }
    

        struct sockaddr_in client;
        int cfd = 0;

        /* Create connection socket and check for errors. */
        if ((cfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            fprintf(stderr, "Error creating connection socket: %s\n", strerror(errno));
            csleep(SECOND);
            continue;
        }

        client.sin_addr = address;
        client.sin_port = htons(remote_port);
        client.sin_family = AF_INET;

        /* Actually connect, checking for errors. */
        if (connect(cfd, (struct sockaddr*) &client, (socklen_t) sizeof(client)) < 0)
        {
            fprintf(stderr, "Error connecting on CONNECT: %s\n", strerror(errno));
            csleep(SECOND);
            continue;
        }

        /* Declare ourselves normal; we are not a CONTROL socket. */
        uint8_t normal_msg = REVSOCKS_NORMAL;
        rsend(cfd, &normal_msg, 1); 

        /* Cool thing: the SOCKS5 protocol implementation doesn't care whether it's a server or a client. */
        struct Client *c = (struct Client*) malloc(sizeof(struct Client));
        c->sock = rs;
        c->addr = client;
        c->fd = cfd;

#ifdef UNIX
        /* Just throw it into a separate thread. NOT using epoll()... */
        pthread_t tmp;
        pthread_create(&tmp, NULL, socks5_client_handler, (void*)c);
#endif

/* Nice... Windows... */
#ifdef WINDOWS
        HANDLE tmpthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) socks5_client_handler, 
                        (void*)c, 0, NULL);
#endif

    }

}

void free_socks5_server(RevSocks *rs)
{
    free(rs);
}