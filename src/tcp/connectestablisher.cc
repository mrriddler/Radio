//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "connectestablisher.h"

#include <algorithm>

#include "tcpclientnotifier.h"
#include "time.h"

typedef std::pair<uint32_t, int32_t> RaceRule;

/*
 *  There are 2 factors to influence caculate sleep_time:
 *  1.interval, try to wakeup every interval.
 *  2.min_connecting_timeoout, wakeup at any connecting timeout.
 *  If there is a conflict, choose 2.
 */

class RaceStrategy {
public:
    RaceStrategy(const uint32_t total_connecting_count);
    RaceRule makeRule(uint32_t connecting_count, int32_t min_connecting_timeout);
    void encounterConnectErrno(int32_t _errno);
private:
    uint32_t interval_;
    uint32_t step_;
    uint64_t pre_connecting_time_;
    int32_t errno_;
    
    const uint32_t total_connecting_count_;
    
    uint32_t caculateProgressiveInterval(uint32_t pre_interval, int32_t _errno);
    uint32_t caculateProgressiveMaxConnectingCount(uint32_t connecting_count, int32_t _errno);
    uint64_t caculatePreConnectingTime();
};

RaceStrategy::RaceStrategy(const uint32_t total_connecting_count):
total_connecting_count_(total_connecting_count) {
    interval_ = kConnectInterval;
    step_ = 0;
    pre_connecting_time_ = 0;
    errno_ = 0;
};

void RaceStrategy::encounterConnectErrno(int32_t _errno) {
    errno_ = _errno;
}

uint32_t RaceStrategy::caculateProgressiveInterval(uint32_t pre_interval, int32_t _errno) {
    return kConnectInterval;
}

uint32_t RaceStrategy::caculateProgressiveMaxConnectingCount(uint32_t connecting_count, int32_t _errno) {
    return kMaxConnectingCount;
}

uint64_t RaceStrategy::caculatePreConnectingTime() {
    if (pre_connecting_time_ > 0) {
        return pre_connecting_time_;
    }
    
    return gettickcount() - kConnectInterval;
}

RaceRule RaceStrategy::makeRule(uint32_t connecting_count, int32_t min_connecting_timeout) {
    uint32_t next_interval = caculateProgressiveInterval(interval_, errno_);
    
    if (step_ >= total_connecting_count_ || connecting_count >= caculateProgressiveMaxConnectingCount(connecting_count, errno_)) {
        interval_ = next_interval;
        return std::make_pair(step_, std::min((int32_t)interval_, min_connecting_timeout));
    }
    
    int32_t elapse_time = interval_ - (int32_t)(gettickcount() - caculatePreConnectingTime());
    if (elapse_time > 0) {
        next_interval = std::min((uint32_t)elapse_time, next_interval);
    }
    
    errno_ = 0;
    pre_connecting_time_ = gettickcount();
    step_++;
    interval_ = next_interval;
    return std::make_pair(step_, std::min((int32_t)interval_, min_connecting_timeout));
}

static bool __isConnecting(const TCPClientNotifier* connect) {
    return connect->currentStatus() == TCPClient::kStatusEnd;
}

TCPClientNotifier* ConnectEstablisher::race(EventLoop& loop,
                                            std::vector<SocketAddress>& addrs,
                                            TCPClientObserver *observer) {
    TCPClientNotifier *winner = NULL;
    
    std::vector<TCPClientNotifier *> competitors;
    for (uint32_t i = 0; i < addrs.size(); ++i) {
        TCPClientNotifier *connect = new TCPClientNotifier(loop, addrs[i].addr(), observer, kConnectTimeout, i);
        competitors.push_back(connect);
    }
    
    RaceStrategy strategy = RaceStrategy((int32_t)competitors.size());
    
    do {
        int32_t min_connecting_timeout = kConnectTimeout;
        uint32_t connecting_count = (uint32_t)std::count_if(competitors.begin(), competitors.end(), &__isConnecting);
        
        for (uint32_t i = 0; i < competitors.size(); ++i) {
            if (NULL == competitors[i]) {
                continue;
            }
            
            TCPClientNotifier *connect = competitors[i];
            min_connecting_timeout = std::min(min_connecting_timeout, connect->timeout());
        }
        
        RaceRule rule = strategy.makeRule(connecting_count, min_connecting_timeout);
        uint32_t step = rule.first;
        int32_t sleep_time = rule.second;
        
        for (uint32_t i = 0; i < step; ++i) {
            if (NULL == competitors[i]) {
                continue;
            }
            
            TCPClientNotifier *connect = competitors[i];
            connect->connect();
        }
        
        loop.sleep(sleep_time);
        
        for (uint32_t i = 0; i < step; ++i) {
            if (NULL == competitors[i]) {
                continue;
            }
            
            if (TCPClient::kStatusEnd == competitors[i]->currentStatus()) {
                int32_t _errno = competitors[i]->getErrno();
                competitors[i]->close();
                delete competitors[i];
                competitors[i] = NULL;
                strategy.encounterConnectErrno(_errno);
            }
            
            if (TCPClient::kStatusRecvOrSend == competitors[i]->currentStatus() &&
                TCPClientNotifier::kStatusNotPass == competitors[i]->currentVerifyStatus()) {
                competitors[i]->close();
                delete competitors[i];
                competitors[i] = NULL;
                strategy.encounterConnectErrno(-1);
            }
            
            if (TCPClient::kStatusRecvOrSend == competitors[i]->currentStatus() &&
                TCPClientNotifier::kStatusPass == competitors[i]->currentVerifyStatus()) {
                winner = competitors[i];
                delete competitors[i];
                competitors[i] = NULL;
            }
        }
        
        bool alive = false;
        for (uint32_t i = 0; i < competitors.size(); ++i) {
            if (NULL != competitors[i]) {
                alive = true;
                break;
            }
        }
        
        if (!alive || NULL != winner) {
            break;
        }
    } while (true);
    
    for (uint32_t i = 0; i < competitors.size(); ++i) {
        if (NULL == competitors[i]) {
            continue;
        }
        
        competitors[i]->close();
        delete competitors[i];
        competitors[i] = NULL;
    }
    
    return winner;
}
