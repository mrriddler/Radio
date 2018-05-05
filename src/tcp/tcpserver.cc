//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "tcpserver.h"

TCPServer::TCPServer(EventLoop& loop, const sockaddr& addr, int backlog):TCPEventSource(loop), addr_(&addr), backlog_(backlog) {
    status_ = kStatusStart;
    pre_status_ = kStatusStart;
    close_direction_ = kDirectionSelfEnd;
    
    ident_ = INVALID_SOCKET_IDENT;
    errno_ = 0;
    
    send_buffer_ = AutoBuffer();
    recv_buffer_ = AutoBuffer();
}

TCPServer::~TCPServer() {
    close(false);
}

void TCPServer::listen() {
    if (status_ != kStatusStart) {
        return;
    }
    
    onCreate();

    ident_ = socket(AF_INET, SOCK_STREAM, 0);
    errno_ = 0;
    
    status_ = kStatusStart;
    pre_status_ = kStatusStart;
    
    if (INVALID_SOCKET_IDENT == ident_) {
        errno_ = socket_errno(ident_);
        transitionFromStartToEnd();
        onLost(pre_status_, errno_);
        return;
    }
    
    if (0 > socket_reuseaddr(ident_, 1)) {
        errno_ = socket_errno(ident_);
        transitionFromStartToEnd();
        
        ::socket_close(ident_);
        ident_ = INVALID_SOCKET_IDENT;

        onLost(pre_status_, errno_);
        return;
    } else {
        transitionFromStartToBind();
    }
    
    if (0 > bind(ident_, &addr_.addr(), addr_.addr_len())) {
        errno_ = socket_errno(ident_);
        transitionFromBindToEnd();
        
        ::socket_close(ident_);
        ident_ = INVALID_SOCKET_IDENT;
        
        onLost(pre_status_, errno_);
        return;
    } else {
        transitionFromBindToListen();
        onBind();
    }
    
    attach();
    
    if (0 > ::listen(ident_, backlog_)) {
        errno_ = socket_errno(ident_);
        transitionFromListenToEnd();
        
        ::socket_close(ident_);
        ident_ = INVALID_SOCKET_IDENT;
        
        detach();
        
        onLost(pre_status_, errno_);
        return;
    } else {
        transitionFromListenToAccept();
        onListen();
    }
}

void TCPServer::processEvent(int ident, int mask) {
    if (TCP_EVENT_SOURCE_WIRTE(mask)) {
        if (true == send_buffer_.empty()) {
            onSend(send_buffer_, send_buffer_.left());
        }
        
        ssize_t ret = ::send(ident_, send_buffer_.getPtr(), send_buffer_.getLength(), 0);
        
        if (0 < ret) {
            send_buffer_.detach(-ret);
        } else {
            goto lost_recv_or_send;
        }
    }
    
    if (TCP_EVENT_SOURCE_READ(mask)) {
        
        //  accept
        if (ident_ == ident) {
            
            struct sockaddr_in client_addr = {0};
            socklen_t client_addr_len = sizeof(client_addr);

            SOCKET_IDENT client_ident = accept(ident_, (struct sockaddr*)&client_addr, &client_addr_len);
            if (INVALID_SOCKET_IDENT != client_ident) {
                transitionFromAcceptToRecvOrSend();
                SocketAddress addr = SocketAddress((const sockaddr*)&client_addr);
                onAccept(client_ident, addr);
            }
            
        //  recv
        } else {
            recv_buffer_.checkCapacity();

            ssize_t ret = ::recv(ident_, ((unsigned char *)recv_buffer_.getPtr() + recv_buffer_.getLength()), recv_buffer_.left(), 0);
            
            // recv data
            if (0 < ret) {
                recv_buffer_.setLength(recv_buffer_.getLength() + ret);
                onRecv(recv_buffer_, ret);
            
            //  recv data encounter error
            } else {
                goto lost_recv_or_send;
            }
        }
    }
    
    if (!TCP_EVENT_SOURCE_READ(mask) && !TCP_EVENT_SOURCE_WIRTE(mask)) {
        goto lost_recv_or_send;
    }
    
lost_recv_or_send:
    errno_ = socket_errno(ident_);
    transitionFromRecvOrSendToEnd();
    
    ::socket_close(ident_);
    ident_ = INVALID_SOCKET_IDENT;
    
    detach();
    
    onLost(pre_status_, errno_);
}

void TCPServer::processError() {
    errno_ = socket_errno(ident_);
    
    switch (status_) {
        case kStatusStart:
            transitionFromStartToEnd();
            break;
        case kStatusBind:
            transitionFromBindToEnd();
            break;
        case kStatusListen:
            transitionFromListenToEnd();
            break;
        case kStatusAccept:
            transitionFromAcceptToEnd();
            break;
        case kStatusRecvOrSend:
            transitionFromRecvOrSendToEnd();
            break;
        default:
            break;
    }
    
    onLost(pre_status_, errno_);
    
    ::socket_close(ident_);
    ident_ = INVALID_SOCKET_IDENT;
    
    detach();
}

void TCPServer::close(bool needOn) {
    if (INVALID_SOCKET_IDENT == ident_) {
        return;
    }
    
    ::socket_close(ident_);
    ident_ = INVALID_SOCKET_IDENT;
    
    detach();
    
    switch (status_) {
        case kStatusStart:
            transitionFromStartToEnd();
            break;
        case kStatusBind:
            transitionFromBindToEnd();
            break;
        case kStatusListen:
            transitionFromListenToEnd();
            break;
        case kStatusAccept:
            transitionFromAcceptToEnd();
            break;
        case kStatusRecvOrSend:
            transitionFromRecvOrSendToEnd();
            break;
        default:
            break;
    }
    
    if (needOn) {
        onClose(kDirectionSelfEnd);
    }
}

TCPServer::TCPServerStatus TCPServer::currentStatus() const {
    return status_;
}

int TCPServer::getErrno() const {
    return errno_;
}

int32_t TCPServer::timeout() const {
    return -1;
}

void TCPServer::transitionFromStartToBind() {
    pre_status_ = status_;
    status_ = kStatusBind;
}

void TCPServer::transitionFromBindToListen() {
    pre_status_ = status_;
    status_ = kStatusListen;
}

void TCPServer::transitionFromListenToAccept() {
    pre_status_ = status_;
    status_ = kStatusAccept;
}

void TCPServer::transitionFromAcceptToRecvOrSend() {
    pre_status_ = status_;
    status_ = kStatusRecvOrSend;
}

void TCPServer::transitionFromStartToEnd() {
    pre_status_ = status_;
    status_ = kStatusEnd;
}

void TCPServer::transitionFromBindToEnd() {
    pre_status_ = status_;
    status_ = kStatusEnd;
}

void TCPServer::transitionFromListenToEnd() {
    pre_status_ = status_;
    status_ = kStatusEnd;
}

void TCPServer::transitionFromAcceptToEnd() {
    pre_status_ = status_;
    status_ = kStatusEnd;
}

void TCPServer::transitionFromRecvOrSendToEnd() {
    pre_status_ = status_;
    status_ = kStatusEnd;
}
