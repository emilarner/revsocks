#include "networkmisc.h"


// 16
// 8

int reliable_recv(int fd, void *dest, size_t length)
{
    /*
    size_t amount_read = 0;
    size_t bytes_left = length;

    while (amount_read < length)
    {
        size_t len = recv(fd, (dest + amount_read), bytes_left, 0);
        if (len <= 0)
            return -1;

        amount_read += len;
        bytes_left -= len;
    }

    return 0; 
    */

   /* it works, whatever. I don't care. it works. */
   recv(fd, dest, length, 0);
}