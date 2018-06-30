//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "tcpclient.h"
#include "config.h"

typedef std::pair<const sockaddr&, uint32_t> TCPClientIdentityPair;

class TCPClientObserver {
public:
    virtual void onCreate(TCPClientIdentityPair pair) = 0;
    virtual void onConnecting(TCPClientIdentityPair pair) = 0;
    virtual void onConnected(TCPClientIdentityPair pair, uint64_t rtt) = 0;
    virtual void onRecv(TCPClientIdentityPair pair, AutoBuffer& buffer, ssize_t size) = 0;
    virtual void onSend(TCPClientIdentityPair pair, AutoBuffer& buffer, ssize_t size) = 0;
    virtual void onClose(TCPClientIdentityPair pair, TCPCloseDirection direction) = 0;
    virtual void onLost(TCPClientIdentityPair pair, TCPClient::TCPClientStatus status, int _errno) = 0;
    virtual bool OnShouldVerify(TCPClientIdentityPair pair) = 0;
    virtual bool onVerifyRecv(TCPClientIdentityPair pair, AutoBuffer& buffer) = 0;
    virtual bool onVerifySend(TCPClientIdentityPair pair, AutoBuffer& buffer) = 0;
};

class TCPClientNotifier : public TCPClient {
public:
    enum TCPVerifyStatus {
        kStatusNoNeed,
        kStatusPass,
        kStatusNotPass,
    };
    
    TCPClientNotifier(EventLoop& loop,
                      const sockaddr& addr,
                      TCPClientObserver *observer,
                      const uint32_t connect_timeout,
                      uint32_t tag);
    
    void attachObserver(TCPClientObserver *observer);
    void detachObserver(TCPClientObserver *observer);
    
    TCPVerifyStatus currentVerifyStatus() const;
protected:
    void onCreate();
    void onConnecting();
    void onConnected(uint64_t rtt);
    void onRecv(AutoBuffer& buffer, ssize_t size);
    void onSend(AutoBuffer& buffer, ssize_t size);
    void onClose(TCPCloseDirection direction);
    void onLost(TCPClientStatus status, int _errno);
    
    int32_t connectTimeout() const;
    int32_t connectAbsTimeout() const;
    int32_t recvOrSendTimeout() const;
    int32_t recvOrSendAbsTimeout() const;

private:
    const uint32_t connect_timeout_;
    uint32_t tag_;
    TCPClientObserver *observer_;
    TCPVerifyStatus verify_status_;
    
    TCPClientIdentityPair makeIdentityPair();
};
