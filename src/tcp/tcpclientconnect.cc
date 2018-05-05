//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "tcpclientconnect.h"

TCPClientConnect::TCPClientConnect(EventLoop& loop, bool reusable):loop_(loop), reusable_(reusable) {}

TCPClientConnect::~TCPClientConnect() {
    if (NULL != tcp_) {
        delete tcp_;
        tcp_ = NULL;
    }
    
    if (reusable_ && NULL != heartbeat_) {
        heartbeat_->cancel();
        delete heartbeat_;
        heartbeat_ = NULL;
    }
}

void TCPClientConnect::attachObserver(TCPClientConnectObserver *observer) {
    observer_ = observer;
}

void TCPClientConnect::detachObserver(TCPClientConnectObserver *observer) {
    observer_ = NULL;
}

void TCPClientConnect::connect(std::vector<SocketAddress>& addrs) {
    if (NULL != tcp_) {
        return;
    }
    
    ConnectEstablisher establisher = ConnectEstablisher();
    tcp_ = establisher.race(loop_, addrs, this);
    
    if (reusable_ && NULL != tcp_) {
        heartbeat_ = new SmartHeartBeat(loop_, this);
    }
}

void TCPClientConnect::close() {
    if (NULL == tcp_) {
        return;
    }
    
    // delete tcp pointer on close
    tcp_->close();

    if (reusable_ && NULL != heartbeat_) {
        heartbeat_->cancel();
        delete heartbeat_;
        heartbeat_ = NULL;
    }
}

void TCPClientConnect::onCreate(TCPClientIdentityPair pair) {
    if (NULL != observer_) {
        observer_->onCreate();
    }
}

void TCPClientConnect::onConnecting(TCPClientIdentityPair pair) {
    if (NULL != observer_) {
        observer_->onConnecting();
    }
}

void TCPClientConnect::onConnected(TCPClientIdentityPair pair, uint64_t rtt) {
    if (NULL != observer_) {
        observer_->onConnected(rtt);
    }
}

void TCPClientConnect::onRecv(TCPClientIdentityPair pair, AutoBuffer& buffer, ssize_t size) {
    if (NULL != observer_) {
        observer_->onRecv(buffer, size);
    }
    
    if (reusable_ && NULL != heartbeat_) {
        heartbeat_->onRecv();
    }
}

void TCPClientConnect::onSend(TCPClientIdentityPair pair, AutoBuffer& buffer, ssize_t size) {
    if (NULL != observer_) {
        observer_->onSend(buffer, size);
    }
    
    if (reusable_ && NULL != heartbeat_) {
        heartbeat_->onSend();
    }
}

void TCPClientConnect::onClose(TCPClientIdentityPair pair, TCPCloseDirection direction) {
    if (NULL != observer_) {
        observer_->onClose(direction);
    }
    
    if (NULL != tcp_) {
        delete tcp_;
        tcp_ = NULL;
    }
}

void TCPClientConnect::onLost(TCPClientIdentityPair pair, TCPClient::TCPClientStatus status, int _errno) {
    if (NULL != observer_) {
        observer_->onLost(status, _errno);
    }
    
    close();
}

bool TCPClientConnect::OnShouldVerify(TCPClientIdentityPair pair) {
    if (NULL != observer_) {
        return observer_->OnShouldVerify();
    }
    
    return false;
}

bool TCPClientConnect::onVerifyRecv(TCPClientIdentityPair pair, AutoBuffer& buffer) {
    if (NULL != observer_) {
        return observer_->onVerifyRecv(buffer);
    }
    
    return true;
}

bool TCPClientConnect::onVerifySend(TCPClientIdentityPair pair, AutoBuffer& buffer) {
    if (NULL != observer_) {
        return observer_->onVerifySend(buffer);
    }
    
    return true;
}

void TCPClientConnect::sendHeartBeatPack() {
    if (NULL != observer_) {
        observer_->onSendHeartBeatPack();
    }
}

void TCPClientConnect::heartbeatTimeout() {
    close();
}
