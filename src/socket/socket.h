//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>

#define SOCKET_IDENT int
#define INVALID_SOCKET_IDENT -1

#define SOCKET_ERRNO errno

#define IS_NOBLOCK_CONNECT_ERRNO(errno) (errno == EINPROGRESS)

#define socket_close close

// for non blocking socket get error
int socket_errno(SOCKET_IDENT ident);

int socket_set_nonblock(SOCKET_IDENT ident);

int socket_reuseaddr(SOCKET_IDENT ident, int optval);

int socket_disable_nagle(SOCKET_IDENT ident, int nagle);
