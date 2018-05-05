//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "tcpclientnotifier.h"

class EventLoop;
class SmartHeartBeatMainTimer;
class SmartHeartBeat;

class SmartHeartBeatWaitTimer : public TimerEventSource {
public:
    SmartHeartBeatWaitTimer(EventLoop& loop,
                            SmartHeartBeat *heartbeat,
                            SmartHeartBeatMainTimer *main_timer);
    void processEvent(int ident, int mask);
private:
    SmartHeartBeat *heartbeat_;
    SmartHeartBeatMainTimer *main_timer_;
};

class SmartHeartBeatMainTimer : public TimerEventSource {
public:
    SmartHeartBeatMainTimer(EventLoop& loop, SmartHeartBeat *heartbeat);
    void processEvent(int ident, int mask);
private:
    SmartHeartBeat *heartbeat_;
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
    void cancel();
    
    void onRecv();
    void onSend();
    
    void sendHeartBeatPack();
    void timeout();
    
    int64_t waitInterval(int64_t pre_wait_interval) const;
    int64_t mainInterval(int64_t pre_wait_interval) const;
private:
    SmartHeartBeatDelegate *delegate_;
    
    uint32_t success_count_;
    
    SmartHeartBeatWaitTimer *wait_timer_;
    SmartHeartBeatMainTimer *main_timer_;
};
