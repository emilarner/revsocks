#ifndef NETWORKMISC_H 
#define NETWORKMISC_H

#include "config.h"

#ifdef UNIX
	#include <unistd.h>
	#include <fcntl.h>
	#include <stdio.h>
	#include <pthread.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/signal.h>
	#include <sys/select.h>
	#include <netdb.h>
#endif

#ifdef WINDOWS
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	
	#include <io.h>
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <iphlpapi.h>
	#include <stdio.h>
	#include <signal.h>
	#include <tchar.h>
	#include <strsafe.h>	

	#pragma comment(lib, "Ws2_32.lib")
#endif


ssize_t rrecv(int fd, void *dest, size_t length);
ssize_t rsend(int fd, void *src, size_t length);



#endif