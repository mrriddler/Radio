//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "socketaddress.h"

SocketAddress::SocketAddress(const sockaddr *addr) {}

const sockaddr& SocketAddress::addr() const {
    return (sockaddr&)addr_;
}

const socklen_t SocketAddress::addr_len() const {
    return 0;
}
