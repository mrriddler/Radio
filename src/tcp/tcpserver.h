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
//---- TCP Server State Machine ----//
//
//     ---→   Start -----
//              |        |
//              |        |
//              ↓        |
//             Bind  ----|
//              |        |
//              |        |
//              ↓        |
//           Listen  ----|
//              |        |
//              |        |
//              ↓        |
//           Accept  ----|
//              |        |
//              |        |
//              ↓        |
//          RecvOrSend   |
//              |        |
//              |        |
//              ↓        |
//     ←---    End ←-----
//

class TCPServer : public TCPEventSource {
    
public:
    enum TCPServerStatus {
        kStatusStart,
        kStatusBind,
        kStatusListen,
        kStatusAccept,
        kStatusRecvOrSend,
        kStatusEnd,
    };
    
    TCPServer(EventLoop& loop, const sockaddr& addr, int backlog = 256);
    ~TCPServer();
    
    void listen();
    void close() { close(true); };
    
    TCPServerStatus currentStatus() const;
    int getErrno() const;
    
    int32_t timeout() const;
protected:
    void processEvent(int ident, int mask);
    void processError();

    virtual void onCreate() = 0;
    virtual void onBind() = 0;
    virtual void onListen() = 0;
    virtual void onAccept(SOCKET_IDENT client_ident, SocketAddress& client_addr) = 0;
    virtual void onRecv(AutoBuffer& buffer, ssize_t size) = 0;
    virtual void onSend(AutoBuffer& buffer, ssize_t size) = 0;
    virtual void onClose(TCPCloseDirection direction) = 0;
    virtual void onLost(TCPServerStatus status, int _errno) = 0;
    
    TCPServerStatus status_;
    TCPServerStatus pre_status_;
    TCPCloseDirection close_direction_;
    
    SOURCE_IDENT ident_;
    int errno_;
    const int backlog_;

    AutoBuffer send_buffer_;
    AutoBuffer recv_buffer_;

    SocketAddress addr_;
    
private:
    void close(bool needOn);

    void transitionFromStartToBind();
    void transitionFromBindToListen();
    void transitionFromListenToAccept();
    void transitionFromAcceptToRecvOrSend();
    void transitionFromStartToEnd();
    void transitionFromBindToEnd();
    void transitionFromListenToEnd();
    void transitionFromAcceptToEnd();
    void transitionFromRecvOrSendToEnd();
};
