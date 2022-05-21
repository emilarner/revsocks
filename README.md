# revsocks

Cross-platform SOCKS5 proxy server written in C that can also reverse itself over a firewall.

  
  

Version 1.5: Library + Bug fixes

  
  

Changelog:

Version 1.5:

  

- revsocks has been modified such that it can now be a library.

  

Version 1.4:

  

- DNS override implemented. Have certain domains resolve to certain IP addresses given a DNS override file.

- SOCKS5 username-password authentication implemented.

Version 1.3:

  

- Text reverse protocol changed to binary.

- Bug fixes

- Speed enhancement

  

Version 1.2:

  

- Windows support added. As said before, there is a minimal amount of things that need to be changed for a port.

  

Version 1.1:

  

- There are only two servers which run now on the reverse server: remote and local.

- Some infinite loop bugs have been (mostly) fixed.

  

- Implementation of recvall() changed.

  
  

Bugs:

  

- Sometimes, for whatever reason, revsocks will refuse to work on certain services. For instance, running revsocks to reverse proxy a replit.com instance will cause the program to crash after only a few requests have been made through it.

- For some unknown reason, there are instances of infinite loops using insane amounts of system resources. Interestingly, it mostly happens on Windows, whereas on Linux it is quite rare.

  
  

Revsocks is a minimal SOCKS5 proxy server that can either run as an average server or reverse itself over a firewall. Revsocks does not support UDP or BIND features, which are specified in the SOCKS5 protocol specification. Revsocks is not scalable, nor does it employ particularly good programming practices to ensure longevity. Revsocks is to be used for temporary purposes, not for outside the usage of more than about three people. Revsocks uses select() for its purposes, contributing to its lack of scalability.

  

In order to reverse the SOCKS5 proxy server, you need a server on another computer which is accessible over the firewall, so that the computer you wish to host the main proxy server on can access it. The server over the firewall will, itself, host a local SOCKS5 proxy server on its end, so that it may seamlessly use the remote computer as a proxy server.

  

For example, I have computer A which I want to use a proxy, but is not reachable due to a firewall. I would need the *revsocks* application on computer A to connect to computer B, which is hosting the *revsocks* firewall hopper server. Through computer B, I would connect to the locally hosted port such that it will eventually connect through to computer A. The internal workings of *revsocks* are heavily based off of that of *firehop*, which is a similar program that I developed a while ago.

  

Here is the help menu from *revsocks*:

      
    
    revsocks -- A cross-platform, minimal SOCKS5 proxy server, which can be reversed.
    
    NOTE: IPv6 is not supported. UDP and BIND functions are not either.
    
    USAGE:
    
    -q, --quiet Quiet mode. No output will be shown.
    
    -up, --username-password Username and password instead of no auth.
    
    -u, --username [username] Specify the username required for entry.
    
    -p, --password [password] Specify the password required for entry..
    
    -r, --reverse [remote_addr] [remote_port] Reverse the server over the firewall to a remote server.
    
    ~~~~~^ use this if you want to host a SOCKS5 proxy server over a firewall.
    
    ~~~~~^ DEFAULT: not reversed.
    
    -po, --port [port] Port to host on for normal SOCKS5 server.
    
    -rs, --remote-server [remote_port] [lport] Setup a reverse server with the parameters specified.
    
    ~~~~~~~~^ use this to get the SOCKS5 proxy server over the firewall.
    
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^ lport means local port; default not server.
    
    -d, --dns [filename] Modify DNS resolutions using domain:address\n format
    
    -h, --help Display this help menu.

  
  

This program is mostly for my usage, explaining the lack of clarity in my explanations. In general, use *--remote-server* to host a server to help computer A get over the firewall. Use *--reverse* to reverse a SOCKS5 proxy server over the firewall to computer B. *lport* is the port which you would connect to locally on computer B to access the reversed SOCKS5 proxy session. *remote_port* is the port which must be made accessible to computer A to get past the firewall. The naming conventions are ambiguous and confusing, but it is too late to change them now.

  

In order to require username-password authentication, one must first set the *-up, --username-password* flag--otherwise, it will not work. Then, the username and password are promptly specified by the *-u/--username* and *-p/--password* arguments. Unless there is a demand or a need for multiple usernames and passwords, *revsocks* will only support ONE username-password pair for authentication. *revsocks* is not a professional-grade SOCKS5 proxy, after all.

  

*-d/--dns* requires a file which is in the format that *revsocks* can understand correctly. A DNS override file should contain a domain separated by an equals sign *=* to its corresponding IP address, with each entry separated by a UNIX newline *\n*. For reference, please view the *example-dns* file that is within this repository.

  
  
  
  

To compile:

  

On Linux/UNIX machines:

Mark the make.sh script as executable (*chmod 0755 make.sh*), check it, and then run it. The output file, *revsocks*, is your compiled program.

  

On Windows:

You need MSVC and the developer command prompt that comes with it. After entering the developer command prompt, run the make.bat file within this repository. The *revsocks.exe* file is your compiled Windows program.

  

  
To compile as a static library:
On Linux/UNIX machines:

As always, mark the make-lib.sh file as executable, so that you can run it. After running it, you now will have a file at lib/unix called librevsocks.a. In order to compile with the shared library, you must include it in the list of files you are compiling with, like so: `gcc -o main main.c librevsocks.a`. You will also need to include the necessary headers in your C program: `revsocksl.h` or `revsocksserver.h`, which depend on your usage of revsocks as a library. 


Using revsocks as a library:

Read the header files, because they are heavily commented; they will explain everything you need to know. The functions which are commented in the headers are the ones which you will use. Do not use functions which are not commented, as they are meant to be used as private internal functions.  

C++ bindings will not be made because they are unnecessary. You are only calling functions when you use revsocks as a library--it is not that bad, so you do not need object-oriented code for this. You might have to deal with some pointers, though!