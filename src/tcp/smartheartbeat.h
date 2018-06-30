//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "tcpclientnotifier.h"

class EventLoop;
class SmartHeartBeat;

class SmartHeartBeatStrategy {
public:
    void start();
    void end();
    
    void recvHeartResponse();
    uint64_t stepOnInterval();
    uint64_t getCurInterval() const;
private:
    bool probe(bool success);
    void adjustOnSuccess();
    void adjustOnFailure();
    void moveupStep();
    void revertStep();
    
    bool is_waiting_heart_response_;
    bool is_stable_;
    
    uint64_t probe_count_;
    
    uint64_t continuous_success_count_;
    uint64_t continuous_failure_count_;
    
    uint64_t cur_interval_;
};

class SmartHeartBeatTimer : public TimerEventSource {
public:
    SmartHeartBeatTimer(EventLoop& loop,
                        SmartHeartBeat *heartbeat,
                        SmartHeartBeatStrategy *strategy);
    
    void processEvent(int ident, int mask);
    void processError();
private:
    SmartHeartBeat *heartbeat_;
    SmartHeartBeatStrategy *strategy_;
};

class SmartHeartBeatDelegate {
public:
    virtual void sendHeartBeatPack() = 0;
    virtual void heartbeatTimeout() = 0;
};

class SmartHeartBeat {
public:
    SmartHeartBeat(EventLoop& loop, SmartHeartBeatDelegate *delegate);
    ~SmartHeartBeat();
    
    void start();
    void end();
    
    void recvHeartBeatPack();
    
    void sendHeartBeatPack();
    void timeout();
    
private:
    SmartHeartBeatDelegate *delegate_;
    SmartHeartBeatStrategy *strategy_;
    SmartHeartBeatTimer *timer_;
};
