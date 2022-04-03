#ifndef CONFIG_H
#define CONFIG_H

#include "definitions.h"

/* Probably not very useful to change. */
#define LISTEN_BACKLOG 64
#define MAX_SELECT_FDS 1024
#define SELECT_TIMEOUT 360 // In seconds
#define EXCHANGE_BUFFER_SIZE 32768 // in bytes 

/* After an error in a loop, how long should we block for? */
#define ERROR_SLEEP 1 * SECOND



#endif