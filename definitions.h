#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define UNIX
#define LINUX
//#define WINDOWS

#define SECOND 1000000

#ifdef UNIX
#define BITBUCKET "/dev/null"
#endif

#ifdef WINDOWS
#define BITBUCKET "NUL"
#endif

#include <stdint.h>

/* Based off of RFC1928, which is the protocol specification for SOCKS5 */
/* Thanks for not specifying that there is no auth success reply for no authentication! */
/* It offset everything, causing me to question reality for several hours! */
/* Also, thanks for having the username-password authentication in a SEPARATE RFC! */
/* Really helpful! */

enum SOCKSVersion
{
    SOCKS5 = 0x05
};

/* Available authentication methods. See RFC1928 for more details regarding the reserved ones. */
enum MethodTypes
{
    NoAuthentication = 0x00,
    GSSAPI = 0x01,
    UsernamePassword = 0x02,
    NoAcceptableMethods = 0xFF
};

/* We technically do not have to use packed structs. */
/* The protocol accounts for struct word-length alignment, by having reserved fields, */
/* such that the size of the struct is a power of 2. */

/* Also, why is sizeof(enum x) = 4? Most of the time when using enums, it is for byte options? */
/* I accidentally had one struct field be that of the enum type, causing it to be 4 bytes, which offset */
/* literally everything. enums with only byte fields should be size uint8_t! */


/* The client will send this first. It will indicate which methods it can accept for authentication. */
/* Subsequent to this request, multiple methods must be sent, the number of which is indicated by
nomethods. */
struct __attribute__((__packed__)) ClientSelectMethod
{
    uint8_t version;
    uint8_t nomethods;
};

/* The server will reply, giving the suggested authentication method. */
struct __attribute__((__packed__)) ServerMethodSelection
{
    uint8_t version;
    uint8_t selection;
};


struct __attribute__((__packed__)) ClientUserAuthentication
{
    uint8_t version;
    uint8_t username_len;
};

enum AuthStatuses
{
    AuthSuccess,
    AuthFailed = 0xFF
};

/* If you send this when you agreed with no authentication, may you suffer millions of errors! */
struct __attribute__((__packed__)) ServerAuthenticationResponse
{
    uint8_t version;
    uint8_t status;
};

enum ClientCommands
{
    Connect = 0x01,
    Bind = 0x02, // Not implementing that!
    UDP_Associate = 0x03 // Definitely not implementing that! That seems like a nightmare.
};

enum AddressTypes
{
    IPV4 = 0x01,
    DOMAIN = 0x03,
    IPV6 = 0x04
};

/* This is only half of the ClientRequest. You must read the appropriate length and format for the address,
depending on what the address type is. Ports are also in big endian. */
struct __attribute__((__packed__)) ClientRequest
{
    uint8_t version;
    uint8_t command;
    uint8_t reserved; // zero
    uint8_t address_type;
};

/* Okay, so how do I read the address? RTFM! Just kidding! */
/* If IPV4, just read the 4 bytes of the (Big Endian??? The RFC doesn't say!) address, then read the big endian port. */
/* If DOMAIN, read the first byte. That will be the length. Then read the length. No NULL terminator. Then port*/
/* If IPV6, read the 16 bytes of the address. Then read the port. */


enum ServerReplies
{
    Success,
    Failure,
    ConnectionNotAllowed,
    NetworkUnreachable,
    HostUnreachable,
    ConnectionRefused,
    TTLExpired,
    CommandNotSupported,
    AddressTypeNotSupported,
};

struct __attribute__((__packed__)) ServerResponse
{
    uint8_t version;
    uint8_t reply;
    uint8_t reserved; // zero
    uint8_t address_type;
};

/* ^^^ Same as ClientRequest. A variable-length address will be sent. Then, a Big Endian port. */




#endif