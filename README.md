
# revsocks
Cross-platform SOCKS5 proxy server written in C that can also reverse itself over a firewall.

Revsocks is a minimal SOCKS5 proxy server that can either run as an average server or reverse itself over a firewall. Revsocks does not support UDP or BIND features, which are specified in the SOCKS5 protocol specification. Revsocks is not scalable, nor does it employ particularly good programming practices to ensure longevity. Revsocks is to be used for temporary purposes, not for outside the usage of more than about three people. Revsocks uses select() for its purposes, contributing to its lack of scalability. 

In order to reverse the SOCKS5 proxy server, you need a server on another computer which is accessible over the firewall, so that the computer you wish to host the main proxy server on can access it. The server over the firewall will, itself, host a local SOCKS5 proxy server on its end, so that it may seamlessly use the remote computer as a proxy server.

For example, I have computer A which I want to use a proxy, but is not reachable due to a firewall. I would need the *revsocks* application on computer A to connect to computer B, which is hosting the *revsocks* firewall hopper server. Through computer B, I would connect to the locally hosted port such that it will eventually connect through to computer A. The internal workings of *revsocks* are heavily based off of that of *firehop*, which is a similar program that I developed a while ago.

Here is the help menu from *revsocks*:

    revsocks -- A cross-platform, minimal SOCKS5 proxy server, which can be reversed.
    NOTE: IPv6 is not supported. UDP and BIND functions are not either.
    USAGE:
    -q, --quiet                                 Quiet mode. No output will be shown. 
    -up, --username-password                    Username and password instead of no auth.
    -u, --username [username]                   Specify the username required for entry.
    -p, --password [password]                   Specify the password required for entry..
    -r, --reverse  [remote_addr] [remote_port]  Reverse the server over the firewall to a remote server.
    ~~~~~^ use this if you want to host a SOCKS5 proxy server over a firewall.
    ~~~~~^ DEFAULT: not reversed.
    -po, --port [port]                          Port to host on for normal SOCKS5 server.
    -rs, --remote-server [remote_port] [lport]  Setup a reverse server with the parameters specified.
    ~~~~~~~~^ use this to get the SOCKS5 proxy server over the firewall.
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^ lport means local port; default not server.
    -d, --dns [filename]                        Modify DNS resolutions using domain:address\n format
    -h, --help                                  Display this help menu.

Note: the DNS override feature is not currently implemented.
Note: the username and password authentication negotiation is not implemented either.
Note: there is a potential memory dumping bug observed in this program, which is not fixed as of now. 

This program is mostly for my usage, explaining the lack of clarity in my explanations. In general, use *--remote-server* to host a server to help computer A get over the firewall. Use *--reverse* to reverse a SOCKS5 proxy server over the firewall to computer B. *lport* is the port which you would connect to locally on computer B to access the reversed SOCKS5 proxy session. *remote_port* is the port which must be made accessible to computer A to get past the firewall. The naming conventions are ambiguous and confusing, but it is too late to change them now. 

**Note: remote_port and lport must have a gap greater than 1, due to how the internal workings of this program work.**

Windows support will be swiftly added. There is not much to change, considering how select() and other Berkeley socket functions are also present on Windows, through *winsock2.h*. The only main difference is the method of starting threads: Windows does not have *pthread*. Mac OS also fully implements Berkeley sockets, obviously.
