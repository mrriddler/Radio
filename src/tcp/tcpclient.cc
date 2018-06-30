//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "tcpclient.h"
#include "time.h"

TCPClient::TCPClient(EventLoop& loop, const sockaddr& addr):TCPEventSource(loop),addr_(&addr) {
    status_ = kStatusStart;
    pre_status_ = kStatusStart;
    close_direction_ = kDirectionSelfEnd;
    
    ident_ = INVALID_SOCKET_IDENT;
    errno_ = 0;
    
    start_connect_time_ = 0;
    end_connect_time_ = 0;
    
    send_buffer_ = AutoBuffer();
    recv_buffer_ = AutoBuffer();
}

TCPClient::~TCPClient() {
    close(false);
}

/*
 *  total 4 kinds of special cases
 *  1.timeout on connecting (this layer handle timeout after return, upper layer handle
 *                           timeout before return)
 *  2.non-block connecting error
 *  3.remote end close connection
 *  4.eventloop error
 */

void TCPClient::connect() {
    if (status_ != kStatusStart) {
        return;
    }
    
    onCreate();
    
    ident_ = socket(addr_.addr().sa_family, SOCK_STREAM, IPPROTO_TCP);
    socket_disable_nagle(ident_, 1);
    errno_ = 0;
    
    status_ = kStatusStart;
    pre_status_ = kStatusStart;
    close_direction_ = kDirectionSelfEnd;
    
    int ret = 0;
    
    if (INVALID_SOCKET_IDENT == ident_) {
        goto lost_start;
    }
    
    //  set non block connect
    if (0 != socket_set_nonblock(ident_)) {
        goto lost_start;
    }
    
    // attach to eventLoop
    attach();
    
    start_connect_time_ = gettickcount();
    
    ret = ::connect(ident_, &addr_.addr(), addr_.addr_len());
    
    if (0 != ret && !IS_NOBLOCK_CONNECT_ERRNO(SOCKET_ERRNO)) {
        transitionFromStartToConnect();
        onConnecting();
    } else {
        end_connect_time_ = gettickcount();
        goto lost_start;
    }

lost_start:
    errno_ = socket_errno(ident_);
    transitionFromStartToEnd();
    onLost(pre_status_, errno_);
    return;
}

void TCPClient::processEvent(int ident, int mask) {
    
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
        
        recv_buffer_.checkCapacity();
        
        ssize_t ret = ::recv(ident_, ((unsigned char *)recv_buffer_.getPtr() + recv_buffer_.getLength()), recv_buffer_.left(), 0);
    
        //  connect return
        if (0 == ret && kStatusConnect == status_) {
            int32_t timeout = connectTimeout();
            
            //  connecting timeout
            if (0 >= timeout) {
                goto lost_connect;
            } else {
                errno_ = socket_errno(ident_);
                
                // successfully connected
                if (0 == errno_) {
                    end_connect_time_ = gettickcount();
                    transitionFromConnectToRecvOrSend();
                    onConnected(connectRTT());
                    
                    //  connecting encounter error
                } else {
                    goto lost_connect;
                }
            }
            
        //  remote close
        } else if (0 == ret && kStatusRecvOrSend == status_) {
            close_direction_ = kDirectionRemoteEnd;
            close();
            
        //  recv data
        } else if (0 < ret) {
            recv_buffer_.setLength(recv_buffer_.getLength() + ret);
            onRecv(recv_buffer_, ret);
            
        //  connecting encounter error
        } else if (kStatusConnect == status_) {
            goto lost_connect;
            
        //  recv data encounter error
        } else {
            goto lost_recv_or_send;
        }
    }
    
    if (!TCP_EVENT_SOURCE_WIRTE(mask) && !TCP_EVENT_SOURCE_READ(mask)  && kStatusConnect == status_) {
        goto lost_connect;
    }
    
    if (!TCP_EVENT_SOURCE_WIRTE(mask) && !TCP_EVENT_SOURCE_READ(mask)  && kStatusRecvOrSend == status_) {
        goto lost_recv_or_send;
    }

lost_connect:
    end_connect_time_ = gettickcount();
    errno_ = socket_errno(ident_);
    transitionFromConnectToEnd();
    onLost(pre_status_, errno_);
lost_recv_or_send:
    errno_ = socket_errno(ident_);
    transitionFromRecvOrSendToEnd();
    onLost(pre_status_, errno_);
}

void TCPClient::processError() {
    
    errno_ = socket_errno(ident_);
    
    switch (status_) {
        case kStatusStart:
            transitionFromStartToEnd();
            break;
        case kStatusConnect:
            end_connect_time_ = gettickcount();
            transitionFromConnectToEnd();
            break;
        case kStatusRecvOrSend:
            transitionFromRecvOrSendToEnd();
            break;
        default:
            break;
    }
    
    onLost(pre_status_, errno_);
}

void TCPClient::close(bool needOn) {
    if (INVALID_SOCKET_IDENT == ident_) {
        return;
    }
    
    ::socket_close(ident_);
    ident_ = INVALID_SOCKET_IDENT;
    
    switch (status_) {
        case kStatusStart:
            transitionFromStartToEnd();
            break;
        case kStatusConnect:
            transitionFromConnectToEnd();
            break;
        case kStatusRecvOrSend:
            transitionFromRecvOrSendToEnd();
            break;
        default:
            break;
    }
    
    detach();
    
    if (needOn) {
        onClose(close_direction_);
    }
}

int32_t TCPClient::timeout() const {
    switch (status_) {
        case kStatusStart:
            return -1;
        case kStatusConnect:
            return connectTimeout();
        case kStatusRecvOrSend:
            return recvOrSendTimeout();
        case kStatusEnd:
            return -1;
    }
}

int32_t TCPClient::connectTimeout() const {
    return -1;
}

int32_t TCPClient::connectAbsTimeout() const {
    return -1;
}

int32_t TCPClient::recvOrSendTimeout() const {
    return -1;
}

int32_t TCPClient::recvOrSendAbsTimeout() const {
    return -1;
}

int32_t TCPClient::connectRTT() const {
    return (int32_t)(end_connect_time_ - start_connect_time_);
}

TCPClient::TCPClientStatus TCPClient::currentStatus() const {
    return status_;
}

int TCPClient::getErrno() const {
    return errno_;
}

void TCPClient::transitionFromStartToConnect() {
    pre_status_ = status_;
    status_ = kStatusConnect;
}

void TCPClient::transitionFromConnectToRecvOrSend() {
    pre_status_ = status_;
    status_ = kStatusRecvOrSend;
}

void TCPClient::transitionFromStartToEnd() {
    pre_status_ = status_;
    status_ = kStatusEnd;
}

void TCPClient::transitionFromConnectToEnd() {
    pre_status_ = status_;
    status_ = kStatusEnd;
}

void TCPClient::transitionFromRecvOrSendToEnd() {
    pre_status_ = status_;
    status_ = kStatusEnd;
}
