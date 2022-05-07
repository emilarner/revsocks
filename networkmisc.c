#include "networkmisc.h"

ssize_t rrecv(int fd, void *dest, size_t length)
{
    ssize_t total = 0;

    while (total < length)
    {
        ssize_t len = recv(fd, ((char*)dest) + total, length - total, 0);

        if (len <= 0)
            return -1;

        total += len;
    }

    return total; 
}

ssize_t rsend(int fd, void* src, size_t length)
{
    ssize_t sent = 0;

    while (sent < length)
    {
        ssize_t wrote = send(fd, ((char*) src) + sent, length - sent, 0);

        if (wrote <= 0)
            return -1;

        sent += wrote;
    }

    return sent;
}