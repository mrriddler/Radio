//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "smartheartbeat.h"
#include "activitymonitor.h"

void SmartHeartBeatStrategy::start() {
    probe_count_ = 0;
    is_waiting_heart_response_ = true;
}

void SmartHeartBeatStrategy::end() {
    
    if (is_waiting_heart_response_) {
        adjustOnFailure();
    }
    
    is_waiting_heart_response_ = false;
    continuous_success_count_ = 0;
    probe_count_ = 0;
}

void SmartHeartBeatStrategy::recvHeartResponse() {
    is_waiting_heart_response_ = false;
}

uint64_t SmartHeartBeatStrategy::stepOnInterval() {
    bool success = !is_waiting_heart_response_;
    
    //  min interval for foreground
    if (ActivityMonitor::singleton().isForeground()) {
        return success ? kMinHeartInterval : 0;
    }
    
    if (probe(success)) {
        // success
        if (success) {
            adjustOnSuccess();
            // failure
        } else {
            adjustOnFailure();
        }
    }
    
    is_waiting_heart_response_ = success;
    return success ? cur_interval_ : 0;
}

uint64_t SmartHeartBeatStrategy::getCurInterval() const {
    return cur_interval_;
}

bool SmartHeartBeatStrategy::probe(bool success) {
    probe_count_ += success;
    
    return probe_count_ > kHeartProbeCount;
}

void SmartHeartBeatStrategy::adjustOnSuccess() {
    continuous_failure_count_ = 0;
    continuous_success_count_++;
    
    if (cur_interval_ >= kMaxHeartInterval - kHeartAvoidMaxStep) {
        cur_interval_ = kMaxHeartInterval - kHeartAvoidMaxStep;
        is_stable_ = true;
        return;
    }
    
    if (continuous_success_count_ < kMaxHeartContinuousSuccessCount) {
        return;
    }
    
    moveupStep();
}

void SmartHeartBeatStrategy::adjustOnFailure() {
    continuous_success_count_ = 0;
    continuous_failure_count_++;
    
    if (cur_interval_ <= kMinHeartInterval) {
        cur_interval_ = kMinHeartInterval;
        return;
    }
    
    if (continuous_failure_count_ < kMaxHeartContinuousFailureCount) {
        return;
    }
    
    revertStep();
}

void SmartHeartBeatStrategy::moveupStep() {
    cur_interval_ = cur_interval_ + kHeartStep >= kMaxHeartInterval ? kMaxHeartInterval : cur_interval_ + kHeartStep;
    continuous_success_count_ = 0;
}

void SmartHeartBeatStrategy::revertStep() {
    // revert to begin
    if (is_stable_) {
        cur_interval_ = kMinHeartInterval;
        continuous_success_count_ = 0;
        continuous_failure_count_ = 0;
        is_stable_ = false;
    // revert to last step
    } else {
        cur_interval_ = cur_interval_ - kHeartStep > kMinHeartInterval ? cur_interval_ - kHeartStep : kMinHeartInterval;
        continuous_success_count_ = 0;
        continuous_failure_count_ = 0;
        is_stable_ = true;
    }
}

SmartHeartBeatTimer::SmartHeartBeatTimer(EventLoop& loop,
                                         SmartHeartBeat *heartbeat,
                                         SmartHeartBeatStrategy *strategy):
    TimerEventSource(loop),
    heartbeat_(heartbeat),
    strategy_(strategy) {};

void SmartHeartBeatTimer::processEvent(int ident, int mask) {
    TimerEventSource::processEvent(ident, mask);
    
    uint64_t interval = strategy_->stepOnInterval();
    
    if (interval > 0) {
        run(interval);
        heartbeat_->sendHeartBeatPack();
    } else {
        heartbeat_->timeout();
    }
}

void SmartHeartBeatTimer::processError() {
    TimerEventSource::processError();
    
    strategy_->stepOnInterval();
    heartbeat_->timeout();
}

SmartHeartBeat::SmartHeartBeat(EventLoop& loop, SmartHeartBeatDelegate *delegate):delegate_(delegate) {
    strategy_ = new SmartHeartBeatStrategy();
    timer_ = new SmartHeartBeatTimer(loop, this, strategy_);
}

SmartHeartBeat::~SmartHeartBeat() {
    delete strategy_;
    delete timer_;
    strategy_ = NULL;
    timer_ = NULL;
    delegate_ = NULL;
};

void SmartHeartBeat::start() {
    strategy_->start();
    timer_->run(strategy_->getCurInterval());
}

void SmartHeartBeat::end() {
    strategy_->end();
    timer_->cancel();
}

void SmartHeartBeat::recvHeartBeatPack() {
    strategy_->recvHeartResponse();
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
