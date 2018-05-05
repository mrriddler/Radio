//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "socket.h"
#include <fcntl.h>

int socket_errno(SOCKET_IDENT ident) {
    int ret = 0;
    socklen_t len = sizeof(ret);
    if (0 != getsockopt(ident, SOL_SOCKET, SO_ERROR, &ret, &len)) {
        ret = SOCKET_ERRNO;
    }
    return ret;
}

int socket_set_nonblock(SOCKET_IDENT ident) {
    int ret = fcntl(ident, F_GETFL, 0);
    if (ret > 0) {
        long flag = ret | O_NONBLOCK;
        ret = fcntl(ident, F_SETFL, flag);
    }
    return ret;
}

int socket_reuseaddr(SOCKET_IDENT ident, int optval) {
    return setsockopt(ident, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(int));
}
