//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "tcpclientnotifier.h"
#include "time.h"

TCPClientNotifier::TCPClientNotifier(EventLoop& loop,
                                     const sockaddr& addr,
                                     TCPClientObserver *observer,
                                     const uint32_t connect_timeout,
                                     uint32_t tag):
    TCPClient(loop, addr),
    connect_timeout_(connect_timeout),
    tag_(tag) {
        
    observer_ = observer;
    verify_status_ = observer_ ? (observer_->OnShouldVerify(makePair()) ? kStatusNotPass : kStatusPass) : kStatusNoNeed;
}

void TCPClientNotifier::attachObserver(TCPClientObserver *observer) {
    observer_ = observer;
}

void TCPClientNotifier::detachObserver(TCPClientObserver *observer) {
    observer_ = NULL;
}

TCPClientNotifier::TCPVerifyStatus TCPClientNotifier::currentVerifyStatus() const {
    return verify_status_;
}

void TCPClientNotifier::onCreate() {
    if (NULL != observer_) {
        observer_->onCreate(makePair());
    }
}

void TCPClientNotifier::onConnecting() {
    if (NULL != observer_) {
        observer_->onConnecting(makePair());
    }
}

void TCPClientNotifier::onConnected(uint64_t rtt) {
    if (NULL != observer_) {
        observer_->onConnected(makePair(), rtt);
    }
}

void TCPClientNotifier::onRecv(AutoBuffer& buffer, ssize_t size) {
    if (NULL != observer_) {
        observer_->onRecv(makePair(), buffer, size);
    }
    
    if (kStatusPass == verify_status_) {
        return;
    }
    
    verify_status_ = observer_ ? (observer_->onVerifyRecv(makePair(), buffer) ? kStatusPass : kStatusNotPass) : kStatusNoNeed;
}

void TCPClientNotifier::onSend(AutoBuffer& buffer, ssize_t size) {
    if (NULL != observer_) {
        observer_->onSend(makePair(), buffer, size);
    }
    
    if (kStatusPass == verify_status_) {
        return;
    }
    
    verify_status_ = observer_ ? (observer_->onVerifySend(makePair(), buffer) ? kStatusPass : kStatusNotPass) : kStatusNoNeed;
}

void TCPClientNotifier::onClose(TCPCloseDirection direction) {
    if (NULL != observer_) {
        observer_->onClose(makePair(), direction);
    }
}

void TCPClientNotifier::onLost(TCPClientStatus status, int _errno) {
    if (NULL != observer_) {
        observer_->onLost(makePair(), status, _errno);
    }
}

int32_t TCPClientNotifier::connectTimeout() const {
    return (int32_t)(start_connect_time_ + connectAbsTimeout() - gettickcount());
}

int32_t TCPClientNotifier::connectAbsTimeout() const {
    return connect_timeout_;
}

int32_t TCPClientNotifier::recvOrSendTimeout() const {
    return (int32_t)(end_connect_time_ + TCPClient::recvOrSendTimeout() - gettickcount());
}

int32_t TCPClientNotifier::recvOrSendAbsTimeout() const {
    return (int32_t)(std::max(1000, std::min(6 * connectRTT(), connectAbsTimeout() - connectRTT())));
}

TCPClientIdentityPair TCPClientNotifier::makePair() {
    //TCPClientIdentityPair ret = std::pair<const sockaddr&, uint32_t>(addr_.addr(), tag_);
    TCPClientIdentityPair ret = std::make_pair(addr_.addr(), tag_);
    return ret;
}
