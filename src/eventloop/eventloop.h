//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include <sys/types.h>
#include <sys/time.h>

#include <vector>
#include <set>

#define EL_TIMER_SOURCE 1
#define EL_SOCKET_SOURCE 2
#define EL_ALL_SOURCE (EL_TIMER_SOURCE | EL_SOCKET_SOURCE)

#define EL_BEFORE_WAIT_STEP 1
#define EL_AFTER_WAIT_STEP 2
#define EL_BEFORE_PROCESS_STEP 4
#define EL_AFTER_PROCESS_STEP 8
#define EL_ALL_STEP (EL_BEFORE_SLEEP_STEP | EL_AFTER_SLEEP_STEP | EL_BEFORE_PROCESS_STEP | EL_AFTER_PROCESS_STEP)

#define EL_READ_EVENT 1
#define EL_WRITE_EVENT 2
#define EL_TIMER_EVENT 4
#define EL_ERROR_EVENT 8

#define SOURCE_IDENT int

class EventLoop;
class EventSource;

class EventObserver {
public:
    virtual int getStep() const = 0;
    virtual void arrive(EventLoop& loop) = 0;
};

class EventKernel {
public:
    virtual bool addEvent(SOURCE_IDENT ident, int mask, long long data, bool oneshot) = 0;
    virtual bool delEvent(SOURCE_IDENT ident, int mask, long long data) = 0;
    virtual int poll(EventLoop& loop, struct timeval* tval) = 0;
};

//  after socket accept, original ident would split to a new ident
typedef std::pair<SOURCE_IDENT, std::pair<SOURCE_IDENT, int>> ArrivalEvent;
typedef std::set<std::shared_ptr<EventSource>> EventSourceSet;
typedef std::set<std::shared_ptr<EventObserver>> EventObserversSet;

class EventLoop {
public:
    EventLoop();
    static EventLoop& main();
    ~EventLoop();
    
    void loop();
    void stop();
    void sleep(long long interval);
    
    bool addSource(EventSource& source);
    bool delSource(EventSource& source);
    
    bool addObserver(EventObserver& observer);
    bool delObserver(EventObserver& observer);
    
    void arrive(SOURCE_IDENT orginal_ident, SOURCE_IDENT split_ident, int mask);
    
private:
    void notifyBeforeWaitObserver() { notifyObserver(EL_BEFORE_WAIT_STEP); };
    void wait(int flags);
    void notifyAfterWaitObserver() { notifyObserver(EL_AFTER_WAIT_STEP); };
    void notifyBeforeProcessObserver() { notifyObserver(EL_BEFORE_PROCESS_STEP); };
    int processEvents(int flags);
    void notifyAfterProcessObserver() { notifyObserver(EL_AFTER_PROCESS_STEP); };
    
    void kernel();
    long long findNextSourceTimeoutInms();
    void notifyObserver(int step);
    
    void clearArrivalQueue();
    std::shared_ptr<EventSource> findSourceByArrivalEvent(ArrivalEvent event);
    
    bool stop_;
    EventKernel *kernel_;
    EventSourceSet sources_;
    EventObserversSet observers_;
    std::vector<ArrivalEvent> arrival_queue_;
};

class EventSource {
public:
    EventSource(EventLoop& loop):loop_(loop) {};
    SOURCE_IDENT getIdent() const { return ident_; };
    
    void attach() { loop_.addSource(*this); };
    void detach() { loop_.delSource(*this); };
    
    virtual int getMask() const = 0;
    virtual long long getSourceData() const = 0;
    virtual bool getOneShot() const = 0;
    
    virtual void processEvent(int ident, int mask) = 0;
    virtual void processError() = 0;
protected:
    SOURCE_IDENT ident_;
    EventLoop& loop_;
};
