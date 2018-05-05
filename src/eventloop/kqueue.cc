//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include <unistd.h>

#include "kqueue.h"

#define EXPANSION_AMOUNT 2

KQueue::KQueue(int capacity) {
    kqfd_ = kqueue();
    capacity_ = capacity;
    size_ = 0;
#if defined(__LP64__) && __LP64__
    arrival_ = (kevent64_s *)malloc(sizeof(struct kevent64_s) * capacity);
#else
    arrival_ = (kevent *)malloc(sizeof(struct kevent) * capacity);
#endif
}

KQueue::~KQueue() {
    if (0 != kqfd_) {
        close(kqfd_);
    }
    free(arrival_);
}

void KQueue::resizeFiredEvent() {
    
    int check_diff = capacity_;
    
    if (size_ >= capacity_) {
        check_diff = capacity_ * EXPANSION_AMOUNT;
    } else if (size_ < capacity_ / 2) {
        check_diff = capacity_ / EXPANSION_AMOUNT;
    }
    
    if (check_diff != capacity_) {
        capacity_ = check_diff;
#if defined(__LP64__) && __LP64__
        arrival_ = (kevent64_s *)realloc(arrival_, sizeof(struct kevent64_s) * capacity_);
#else
        arrival_ = (kevent *)realloc(arrival_, sizeof(struct kevent) * capacity_);
#endif
    }
}

bool KQueue::addEvent(SOURCE_IDENT ident, int mask, long long data, bool oneshot) {
#if defined(__LP64__) && __LP64__
    struct kevent64_s ke;
#else
    struct kevent ke;
#endif
    
    uint16_t flags = oneshot ? EV_ADD | EV_ENABLE | EV_ONESHOT : EV_ADD | EV_ENABLE;
    
    if (mask & EL_READ_EVENT) {
#if defined(__LP64__) && __LP64__
        EV_SET64(&ke, ident, EVFILT_READ, flags, 0, data, ident, 0, 0);
#else
        EV_SET(&ke, ident, EVFILT_READ, flags, 0, data, ident);
#endif
    } else if (mask & EL_WRITE_EVENT) {
#if defined(__LP64__) && __LP64__
        EV_SET64(&ke, ident, EVFILT_WRITE, flags, 0, data, ident, 0, 0);
#else
        EV_SET(&ke, ident, EVFILT_WRITE, flags, 0, data, ident);
#endif
    } else if (mask & EL_TIMER_EVENT) {
#if defined(__LP64__) && __LP64__
        EV_SET64(&ke, ident, EVFILT_TIMER, flags, 0, data, ident, 0, 0);
#else
        EV_SET(&ke, ident, EVFILT_TIMER, flags, 0, data, ident);
#endif
    }
    
    if (kevent64(kqfd_, &ke, 1, NULL, 0, 0, NULL) == -1) {
        return false;
    }
    
    size_++;
    resizeFiredEvent();
    
    return true;
}

bool KQueue::delEvent(SOURCE_IDENT ident, int mask, long long data) {
#if defined(__LP64__) && __LP64__
    struct kevent64_s ke;
#else
    struct kevent ke;
#endif
    
    if (mask & EL_READ_EVENT) {
#if defined(__LP64__) && __LP64__
        EV_SET64(&ke, ident, EVFILT_READ, EV_DELETE, 0, data, ident, 0, 0);
#else
        EV_SET(&ke, ident, EVFILT_READ, EV_DELETE, 0, data, ident);
#endif
    } else if (mask & EL_WRITE_EVENT) {
#if defined(__LP64__) && __LP64__
        EV_SET64(&ke, ident, EVFILT_WRITE, EV_DELETE, 0, data, ident, 0, 0);
#else
        EV_SET(&ke, ident, EVFILT_WRITE, EV_DELETE, 0, data, ident);
#endif
    } else if (mask & EL_TIMER_EVENT) {
#if defined(__LP64__) && __LP64__
        EV_SET64(&ke, ident, EVFILT_TIMER, EV_DELETE, 0, data, ident, 0, 0);
#else
        EV_SET(&ke, ident, EVFILT_TIMER, EV_DELETE, 0, data, ident);
#endif
    }
    
    if (kevent64(kqfd_, &ke, 1, NULL, 0, 0, NULL) == -1) {
        return false;
    }
    
    size_--;
    resizeFiredEvent();
    
    return true;
}

int KQueue::poll(EventLoop& loop, struct timeval *tvl) {
    int numevents;
    
    if (tvl != NULL) {
        struct timespec timeout;
        timeout.tv_sec = tvl->tv_sec;
        timeout.tv_nsec = tvl->tv_usec * 1000;
#if defined(__LP64__) && __LP64__
        numevents = kevent64(kqfd_, NULL, 0, arrival_, capacity_, 0, &timeout);
#else
        numevents = kevent(kqfd_, NULL, 0, arrival_, capacity_, 0, timeout);
#endif
    } else {
#if defined(__LP64__) && __LP64__
        numevents = kevent64(kqfd_, NULL, 0, arrival_, capacity_, 0, NULL);
#else
        numevents = kevent(kqfd_, NULL, 0, arrival_, capacity_, 0, NULL);
#endif
    }

    if (numevents > 0) {
        for(int i = 0; i< numevents; i++) {
#if defined(__LP64__) && __LP64__
            struct kevent64_s *c_event = arrival_ + i;
#else
            struct kevent *c_event = arrival_ + i;
#endif
            int mask = 0;
            if (c_event->filter == EVFILT_READ && c_event->flags == EV_ERROR) {
                mask |= EL_ERROR_EVENT;
            } else if (c_event->filter == EVFILT_WRITE && c_event->flags == EV_ERROR) {
                mask |= EL_ERROR_EVENT;
            } else if (c_event->filter == EVFILT_WRITE) {
                mask |= EL_WRITE_EVENT;
            } else if (c_event->filter == EVFILT_TIMER) {
                mask |= EL_TIMER_EVENT;
            }
            
            loop.arrive((int)c_event->udata, (int)c_event->ident, mask);
        }
    }
    
    return numevents;
}
