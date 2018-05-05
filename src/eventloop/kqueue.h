//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include <stdlib.h>
#include <sys/event.h>

#include "eventloop.h"

class KQueue : public EventKernel {
public:
    KQueue(int capacity);
    ~KQueue();
    
    bool addEvent(SOURCE_IDENT ident, int mask, long long data, bool oneshot);
    bool delEvent(SOURCE_IDENT ident, int mask, long long data);
    int poll(EventLoop& loop, struct timeval* tvl);

private:
    int kqfd_;
#if defined(__LP64__) && __LP64__
    struct kevent64_s *arrival_;
#else
    struct kevent *arrival_;
#endif
    
    int capacity_;
    int size_;
    void resizeFiredEvent();
};

