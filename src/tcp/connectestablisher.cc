//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "connectestablisher.h"

#include <algorithm>

#include "tcpclientnotifier.h"
#include "time.h"

typedef std::pair<uint32_t, uint64_t> RaceRule;

class RaceStrategy {
public:
    RaceStrategy(const uint32_t total_connecting_count);
    RaceRule makeRule(uint32_t connecting_count);
private:
    uint32_t step_;
    uint64_t pre_connecting_time_;
    
    const uint32_t total_connecting_count_;
};

RaceStrategy::RaceStrategy(const uint32_t total_connecting_count):
total_connecting_count_(total_connecting_count) {
    step_ = 0;
    pre_connecting_time_ = 0;
};

RaceRule RaceStrategy::makeRule(uint32_t connecting_count) {
    uint64_t interval = kRaceConnectInterval;
    
    if (step_ >= total_connecting_count_ || connecting_count >= kMaxConnectingCount) {
        return std::make_pair(step_, interval);
    }
    
    int64_t elapse_time = interval - (gettickcount() - pre_connecting_time_);
    if (elapse_time > 0) {
        interval = std::min(interval, (uint64_t)elapse_time);
    } else {
        interval = std::min(interval, kRaceConnectStep);
    }
    
    pre_connecting_time_ = gettickcount();
    step_++;

    return std::make_pair(step_, interval);
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
        TCPClientNotifier *connect = new TCPClientNotifier(loop, addrs[i].addr(), observer, kRaceConnectInterval, i);
        competitors.push_back(connect);
    }
    
    RaceStrategy strategy = RaceStrategy((int32_t)competitors.size());
    
    do {
        uint32_t connecting_count = (uint32_t)std::count_if(competitors.begin(), competitors.end(), &__isConnecting);
        
        RaceRule rule = strategy.makeRule(connecting_count);
        uint32_t step = rule.first;
        uint64_t sleep_time = rule.second;
        
        for (uint32_t i = 0; i < step; ++i) {
            if (NULL == competitors[i]) {
                continue;
            }
            
            TCPClientNotifier *connect = competitors[i];
            connect->connect();
            
            if (connect->timeout() > 0) {
                sleep_time = std::min(sleep_time, (uint64_t)connect->timeout());
            }
        }
        
        loop.sleep(sleep_time);
        
        for (uint32_t i = 0; i < step; ++i) {
            if (NULL == competitors[i]) {
                continue;
            }
            
            if (TCPClient::kStatusEnd == competitors[i]->currentStatus()) {
                competitors[i]->close();
                delete competitors[i];
                competitors[i] = NULL;
            }
            
            if (TCPClient::kStatusRecvOrSend == competitors[i]->currentStatus() &&
                TCPClientNotifier::kStatusNotPass == competitors[i]->currentVerifyStatus()) {
                competitors[i]->close();
                delete competitors[i];
                competitors[i] = NULL;
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
