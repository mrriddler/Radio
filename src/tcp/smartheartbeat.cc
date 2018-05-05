//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "smartheartbeat.h"

SmartHeartBeatWaitTimer::SmartHeartBeatWaitTimer(EventLoop& loop,
                                                 SmartHeartBeat *heartbeat,
                                                 SmartHeartBeatMainTimer *main_timer):
    TimerEventSource(loop),
    heartbeat_(heartbeat),
    main_timer_(main_timer) {};

void SmartHeartBeatWaitTimer::processEvent(int ident, int mask) {
    TimerEventSource::processEvent(ident, mask);
    
    main_timer_->cancel();
    heartbeat_->sendHeartBeatPack();
    main_timer_->run(heartbeat_->mainInterval(elpaseTime()));
};

SmartHeartBeatMainTimer::SmartHeartBeatMainTimer(EventLoop& loop,
                                                 SmartHeartBeat *heartbeat):
    TimerEventSource(loop),
    heartbeat_(heartbeat) {};

void SmartHeartBeatMainTimer::processEvent(int ident, int mask) {
    TimerEventSource::processEvent(ident, mask);
    
    heartbeat_->timeout();
};

SmartHeartBeat::SmartHeartBeat(EventLoop& loop, SmartHeartBeatDelegate *delegate):delegate_(delegate) {
    main_timer_ = new SmartHeartBeatMainTimer(loop, this);
    wait_timer_ = new SmartHeartBeatWaitTimer(loop, this, main_timer_);
}

SmartHeartBeat::~SmartHeartBeat() {
    delete wait_timer_;
    delete main_timer_;
    wait_timer_ = NULL;
    main_timer_ = NULL;
    delegate_ = NULL;
};

void SmartHeartBeat::start() {
    wait_timer_->cancel();
    wait_timer_->run(waitInterval(wait_timer_->elpaseTime()));
    main_timer_->cancel();
    
    success_count_ = 0;
}

void SmartHeartBeat::cancel() {
    wait_timer_->cancel();
    main_timer_->cancel();
}

void SmartHeartBeat::onRecv() {
    main_timer_->cancel();
    
    success_count_++;
}

void SmartHeartBeat::onSend() {
    wait_timer_->cancel();
    wait_timer_->run(waitInterval(wait_timer_->elpaseTime()));
}

void SmartHeartBeat::sendHeartBeatPack() {
    if (delegate_) {
        delegate_->sendHeartBeatPack();
    }
}

void SmartHeartBeat::timeout() {
    if (delegate_) {
        delegate_->heartbeatTimeout();
    }
}

int64_t SmartHeartBeat::waitInterval(int64_t pre_wait_interval) const {
    return kWaitInterval;
}

int64_t SmartHeartBeat::mainInterval(int64_t pre_wait_interval) const {
    if (pre_wait_interval > kPreWaitIntervalThershold) {
        return kMainShortInterval;
    }
    
    return kMainLongInterval;
}
