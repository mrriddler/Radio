//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include <list>

#include "eventsource.h"
#include "tcpdirection.h"
#include "socketaddress.h"
#include "autobuffer.h"

//
//---- TCP Client State Machine ----//
//
//     ---→   Start -----
//              |        |
//              |        |
//              ↓        |
//           Connect ----|
//              |        |
//              |        |
//              ↓        |
//          RecvOrSend   |
//              |        |
//              |        |
//              ↓        |
//     ←---    End ←-----
//

class TCPClient : public TCPEventSource {
public:
    enum TCPClientStatus {
        kStatusStart,
        kStatusConnect,
        kStatusRecvOrSend,
        kStatusEnd,
    };
    
    TCPClient(EventLoop& loop, const sockaddr& addr);
    ~TCPClient();
    
    void connect();
    void close() { close(true); };
    
    int32_t connectRTT() const;
    int32_t timeout() const;
    
    TCPClientStatus currentStatus() const;
    int getErrno() const;
protected:
    void processEvent(int ident, int mask);
    void processError();
    
    virtual void onCreate() = 0;
    virtual void onConnecting() = 0;
    virtual void onConnected(uint64_t rtt) = 0;
    virtual void onRecv(AutoBuffer& buffer, ssize_t size) = 0;
    virtual void onSend(AutoBuffer& buffer, ssize_t size) = 0;
    virtual void onClose(TCPCloseDirection direction) = 0;
    virtual void onLost(TCPClientStatus status, int _errno) = 0;
    
    virtual int32_t connectTimeout() const;
    virtual int32_t connectAbsTimeout() const;
    virtual int32_t recvOrSendTimeout() const;
    virtual int32_t recvOrSendAbsTimeout() const;
    
    TCPClientStatus status_;
    TCPClientStatus pre_status_;
    TCPCloseDirection close_direction_;
    
    SOURCE_IDENT ident_;
    int errno_;
    
    uint64_t start_connect_time_;
    uint64_t end_connect_time_;
    
    AutoBuffer send_buffer_;
    AutoBuffer recv_buffer_;
    
    SocketAddress addr_;
    
private:
    void close(bool needOn);

    void transitionFromStartToConnect();
    void transitionFromConnectToRecvOrSend();
    void transitionFromStartToEnd();
    void transitionFromConnectToEnd();
    void transitionFromRecvOrSendToEnd();
};
