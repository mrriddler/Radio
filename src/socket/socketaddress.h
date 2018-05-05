//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "socket.h"

class SocketAddress {
public:
    SocketAddress(const sockaddr *addr);
    
    const sockaddr& addr() const;
    const socklen_t addr_len() const;
    
private:
    union {
        struct sockaddr *sa;
        struct sockaddr_in *in;
        struct sockaddr_in6 *in6;
    } addr_;
};
