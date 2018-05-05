//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include <vector>
#include <stdint.h>

class EventLoop;
class TCPClientNotifier;
class TCPClientObserver;
class SocketAddress;

class ConnectEstablisher {
public:
    TCPClientNotifier* race(EventLoop& loop,
                            std::vector<SocketAddress>& addrs,
                            TCPClientObserver *observer);
};

