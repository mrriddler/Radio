//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "connectestablisher.h"
#include "smartheartbeat.h"

class SmartHeartBeat;

class TCPClientConnectObserver {
public:
    virtual void onCreate() = 0;
    virtual void onConnecting() = 0;
    virtual void onConnected(uint64_t rtt) = 0;
    virtual void onRecv(AutoBuffer& buffer, ssize_t size) = 0;
    virtual void onSend(AutoBuffer& buffer, ssize_t size) = 0;
    virtual void onClose(TCPCloseDirection direction) = 0;
    virtual void onLost(TCPClient::TCPClientStatus status, int _errno) = 0;
    virtual bool OnShouldVerify() = 0;
    virtual bool onVerifyRecv(AutoBuffer& buffer) = 0;
    virtual bool onVerifySend(AutoBuffer& buffer) = 0;
    virtual void onSendHeartBeatPack() = 0;
};

class TCPClientConnect : TCPClientObserver, SmartHeartBeatDelegate {
public:
    TCPClientConnect(EventLoop& loop, bool reusable);
    ~TCPClientConnect();
    
    void connect(std::vector<SocketAddress>& addrs);
    void close();
    
    void attachObserver(TCPClientConnectObserver *observer);
    void detachObserver(TCPClientConnectObserver *observer);
    
private:
    void onCreate(TCPClientIdentityPair pair);
    void onConnecting(TCPClientIdentityPair pair);
    void onConnected(TCPClientIdentityPair pair, uint64_t rtt);
    void onRecv(TCPClientIdentityPair pair, AutoBuffer& buffer, ssize_t size);
    void onSend(TCPClientIdentityPair pair, AutoBuffer& buffer, ssize_t size);
    void onClose(TCPClientIdentityPair pair, TCPCloseDirection direction);
    void onLost(TCPClientIdentityPair pair, TCPClient::TCPClientStatus status, int _errno);
    bool OnShouldVerify(TCPClientIdentityPair pair);
    bool onVerifyRecv(TCPClientIdentityPair pair, AutoBuffer& buffer);
    bool onVerifySend(TCPClientIdentityPair pair, AutoBuffer& buffer);
    
    void sendHeartBeatPack();
    void heartbeatTimeout();

    bool reusable_;
    EventLoop& loop_;
    TCPClientNotifier *tcp_;
    TCPClientConnectObserver *observer_;
    SmartHeartBeat *heartbeat_;
};
