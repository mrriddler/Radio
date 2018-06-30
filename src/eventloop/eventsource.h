//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "eventloop.h"
#include "time.h"

#define TCP_EVENT_SOURCE_READ(mask) (mask & EL_READ_EVENT)
#define TCP_EVENT_SOURCE_WIRTE(mask) (mask & EL_WRITE_EVENT)

class TCPEventSource : public EventSource {
public:
    TCPEventSource(EventLoop& loop):EventSource(loop) {};

    int getMask() const { return EL_READ_EVENT | EL_WRITE_EVENT; };
    long long getSourceData() const { return timeout(); };
    bool getOneShot() const { return false; };
    
    virtual void processEvent(int ident, int mask) = 0;
    virtual void processError() = 0;

    virtual int32_t timeout() const = 0;
};

enum TimerEventSourceStatus {
    kStatusStart,
    kStatusRun,
    kStatusCancel,
};

class TimerEventSource : EventSource {
public:
    TimerEventSource(EventLoop& loop):EventSource(loop) { ident_ = 1; };
    ~TimerEventSource() { if (kStatusRun == status_) detach(); }
    
    void run(uint64_t interval) {
        if (kStatusRun == status_) {
            return;
        }
        
        status_ = kStatusRun;
        interval_ = interval;
        start_time_ = gettickcount();
        end_time_ = 0;
        
        attach();
    };
    
    void cancel() {
        if (kStatusRun != status_) {
            return;
        }
        
        status_ = kStatusCancel;
        end_time_ = gettickcount();
        detach();
    };
    
    virtual void processEvent(int ident, int mask) {
        end_time_ = gettickcount();
    };
    
    virtual void processError() {
        end_time_ = gettickcount();
    };
    
    int getMask() const { return EL_TIMER_EVENT; };
    long long getSourceData() const { return interval_; };
    bool getOneShot() const { return true; };
    uint64_t elpaseTime() { return end_time_ - start_time_; };
protected:
    TimerEventSourceStatus status_;
    uint64_t interval_;
    
    uint64_t start_time_;
    uint64_t end_time_;
};

