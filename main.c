#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "networkmisc.h"
#include "revsocksl.h"
#include "revsocksserver.h"

void help()
{
    const char *msg =
    "revsocks -- A cross-platform, minimal SOCKS5 proxy server, which can be reversed.\n"
    "NOTE: IPv6 is not supported. UDP and BIND functions are not either.\n"
    "USAGE:\n"
    "-q, --quiet                                 Quiet mode. No output will be shown. \n"
    "-up, --username-password                    Username and password instead of no auth.\n"
    "-u, --username [username]                   Specify the username required for entry.\n"
    "-p, --password [password]                   Specify the password required for entry..\n"
    "-r, --reverse  [remote_addr] [remote_port]  Reverse the server over the firewall to a remote server.\n"
    "~~~~~^ use this if you want to host a SOCKS5 proxy server over a firewall.\n"
    "~~~~~^ DEFAULT: not reversed.\n"
    "-po, --port [port]                          Port to host on for normal SOCKS5 server.\n"
    "~~~~~~~~~~~~^ by default, port 8080\n"
    "-rs, --remote-server [remote_port] [lport]  Setup a reverse server with the parameters specified.\n"
    "~~~~~~~~^ use this to get the SOCKS5 proxy server over the firewall.\n"
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^ lport means local port; default not server.\n"
    "-d, --dns [filename]                        Modify DNS resolutions using domain:address\\n format\n"
    "-h, --help                                  Display this help menu.\n";

    printf("%s\n", msg);
}

/* Checks if all characters within the port string are digits. */
bool valid_port(char *strport)
{
    for (int i = 0; i < strlen(strport); i++)
    {
        if (!isdigit(strport[i]))
            return false;
    }

    return true;
}

int main(int argc, char **argv, char **envp)
{
#ifdef WINDOWS
        /* winsock requires initialization, for some reason... */

        WSADATA wsaData;
        int iResult;

        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) 
        {
            printf("WSAStartup failed: %d\n", iResult);
            return 1;
        }
#endif

    bool password_auth = false;
    char *username = NULL;
    char *password = NULL;

    char *dns_file = NULL;
    
    int port = 8080;

    bool reversed = false;
    char *raddr = NULL;
    int rport = 0;
    int lport = 0;

    bool rserver = false;

    /* Argument parsing. The minimal way. */
    /* That being said, this turns out to be quite miserable. */
    for (int i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quiet"))
        {
            FILE *fp = fopen(BITBUCKET, "w+");
            dup2(fileno(fp), STDOUT_FILENO);
            dup2(fileno(fp), STDERR_FILENO);
        }

        else if (!strcmp(argv[i], "-up") || !strcmp(argv[i], "--username-password"))
        {
            password_auth = true;
        }

        else if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--username"))
        {
            if (argv[i + 1] == NULL)
            {
                fprintf(stderr, "Error: -u/--username requires an argument.\n");
                return -1;
            }

            username = argv[i + 1];
        }

        else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--password"))
        {
            if (argv[i + 1] == NULL)
            {
                fprintf(stderr, "Error: -p/--password requires an argument.\n");
                return -1;
            }

            password = argv[i + 1];
        }

        else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--reverse"))
        {
            /* Be careful! */
            /* Not using an already existing argparser is starting to suck... */
            for (int j = 1; j < 3; j++)
            {
                if (argv[i + j] == NULL)
                {
                    fprintf(stderr, "Error: -r/--reverse requires multiple arguments.\n");
                    return -1;
                }
            }

            reversed = true;
            raddr = argv[i + 1];

            if (!valid_port(argv[i + 2]))
            {
                fprintf(stderr, "Error: Invalid port!\n");
                return -1;
            }

            rport = atoi(argv[i + 2]);
        }

        else if (!strcmp(argv[i], "-rs") || !strcmp(argv[i], "--remote-server"))
        {
            for (int j = 1; j < 3; j++)
            {
                if (argv[i + j] == NULL)
                {
                    fprintf(stderr, "Error: -rs/--remote-server requires multiple arguments.\n");
                    return -1;
                }
            }

            if (!valid_port(argv[i + 1]) || !valid_port(argv[i + 2]))
            {
                fprintf(stderr, "Error: Invalid ports provided.\n");
                return -1;
            }

            rserver = true;

            rport = atoi(argv[i + 1]);
            lport = atoi(argv[i + 2]);
        }
        else if (!strcmp(argv[i], "-po") || !strcmp(argv[i], "--port"))
        {
            if (argv[i + 1] == NULL)
            {
                fprintf(stderr, "Error: -po/--port requires a port.\n");
                return -1;
            }

            if (!valid_port(argv[i + 1]))
            {
                fprintf(stderr, "Error: an invalid port was provided.\n");
                return -1;
            }

            port = atoi(argv[i + 1]);
        }
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
        {
            help();
            return 0;
        }
        else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--dns"))
        {
            if (argv[i + 1] == NULL)
            {
                fprintf(stderr, "-d/--dns requires the filename of the DNS override file. See docs?\n");
                return -1;
            }

            dns_file = argv[i + 1];
        }
    }
    
    if (rserver)
    {   
        RevSocksServer *srv = init_revsocksserver(rport, lport);
        srv->echo = true;
        
        printf("Hosting a reverse server, with a remote port of %d and a local port of %d\n", 
                rport, lport);

        int status = start_revsocksserver(srv);

        if (status != 0)
            fprintf(stderr, "Error starting RevSocksServer: %s\n", strerror(status));

        free_revsocksserver(srv);
    }
    else
    {
        if (password_auth)
        {
            if (username == NULL || password == NULL)
            {
                fprintf(stderr, "Error: One credential provided, but not the other.\n");
                return -1;
            }
        }

        RevSocks *s = init_socks5_server(username, password, port, dns_file);
        s->echo = true;

        /* Error initializing SOCKS5 server. */
        if (s == NULL)
        {
            fprintf(stderr, "There was an error initializing the SOCKS5 proxy server.\n");
            return -1;
        }

        if (reversed)
        {
            if (raddr == NULL || rport == 0)
            {
                fprintf(stderr, "Error: Parameters required for reverse SOCKS5 not given.\n");
                return -1;
            }

            printf("Hosting a reverse SOCKS5 proxy server over to %s:%d\n", raddr, rport);
            host_rev_socks5_server(s, raddr, rport);
        }
        else
        {
            printf("Hosting a regular SOCKS5 proxy server on port %d\n", port);
            host_socks5_server(s);
        }

        free_socks5_server(s); 
    }

    return 0;
}