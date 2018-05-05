//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include <algorithm>

#include "kqueue.h"

EventLoop::EventLoop() {
    stop_ = false;
    kernel();
};

EventLoop& EventLoop::main() {
    static EventLoop loop;
    loop.loop();
    return loop;
}

EventLoop::~EventLoop() {
    for (EventSourceSet::iterator ite = sources_.begin(); ite != sources_.end(); ++ite) {
        std::shared_ptr<EventSource> source = *ite;
        kernel_->delEvent(source->getIdent(), source->getMask(), source->getSourceData());
    }
    
    delete static_cast<KQueue *>(kernel_);
    kernel_ = NULL;
}

void EventLoop::kernel() {
    kernel_ = new KQueue(10);
};

void EventLoop::loop() {
    stop_ = false;
    
    while (!stop_) {
        notifyBeforeWaitObserver();
        wait(EL_ALL_SOURCE);
        notifyAfterWaitObserver();
        notifyBeforeProcessObserver();
        processEvents(EL_ALL_SOURCE);
        notifyAfterProcessObserver();
    }
};

void EventLoop::stop() {
    stop_ = true;
}

void EventLoop::sleep(long long interval) {
    EventKernel *sleep_kernel = new KQueue(1);
    
    sleep_kernel->addEvent(1, EL_TIMER_EVENT, interval + 1, true);
    
    struct timeval tval;
    tval.tv_sec = interval / 1000;
    tval.tv_usec = (interval % 1000) * 1000;
    
    sleep_kernel->poll(*(this), &tval);
    sleep_kernel->delEvent(1, EL_TIMER_EVENT, interval + 1);
    
    delete static_cast<KQueue *>(sleep_kernel);
    sleep_kernel = NULL;
}

void EventLoop::wait(int flags) {
    struct timeval tval;
    
    if (flags & EL_TIMER_SOURCE || flags & EL_SOCKET_SOURCE) {
        // find next source to timeout
        long long ms = findNextSourceTimeoutInms();
        if (ms > 0) {
            tval.tv_sec = ms / 1000;
            tval.tv_usec = (ms % 1000) * 1000;
        } else {
            // wait forever
            tval.tv_sec = 0;
            tval.tv_usec = 0;
        }
    } else {
        // wait forever
        tval.tv_sec = 0;
        tval.tv_usec = 0;
    }
    
    kernel_->poll(*this, &tval);
}

int EventLoop::processEvents(int flags) {
    
    if ((flags & EL_TIMER_SOURCE) == 0 && (flags & EL_SOCKET_SOURCE) == 0) {
        return 0;
    }
    
    int processed = 0;
    
    for(std::vector<ArrivalEvent>::iterator ite = arrival_queue_.begin(); ite != arrival_queue_.end(); ite++) {
        ArrivalEvent event = *ite;
        std::shared_ptr<EventSource> source = findSourceByArrivalEvent(event);

        // IO for socket source
        if (flags & EL_SOCKET_SOURCE &&
            (event.second.second & EL_READ_EVENT || event.second.second & EL_WRITE_EVENT) &&
            source) {
            source->processEvent(event.second.first, event.second.second);
            processed++;
        // Error for socket source
        } else if (flags & EL_SOCKET_SOURCE &&
                   (event.second.second & EL_ERROR_EVENT) &&
                   source) {
            source->processError();
            processed++;
        // Fire for timer source
        } else if (flags & EL_TIMER_SOURCE &&
                   event.second.second & EL_TIMER_EVENT &&
                   source) {
            source->processEvent(event.second.first, event.second.second);
            processed++;
        // Error for timer source
        } else if (flags & EL_TIMER_SOURCE &&
             event.second.second & EL_ERROR_EVENT &&
             source) {
            source->processError();
            processed++;
        }
    }
        
    clearArrivalQueue();
    
    return processed;
}

bool EventLoop::addSource(EventSource& source) {
    std::shared_ptr<EventSource> source_ptr = std::shared_ptr<EventSource>(&source);
    if (sources_.find(source_ptr) != sources_.end()) {
        return false;
    }

    sources_.insert(source_ptr);
    return kernel_->addEvent(source_ptr->getIdent(), source_ptr->getMask(), source_ptr->getSourceData(), source_ptr->getOneShot());
}

bool EventLoop::delSource(EventSource& source) {
    std::shared_ptr<EventSource> source_ptr = std::shared_ptr<EventSource>(&source);
    if (sources_.find(source_ptr) != sources_.end()) {
        return false;
    }
    
    sources_.erase(source_ptr);
    
    return kernel_->delEvent(source_ptr->getIdent(), source_ptr->getMask(), source_ptr->getSourceData());
}

bool EventLoop::addObserver(EventObserver& observer) {
    std::shared_ptr<EventObserver> observer_ptr = std::shared_ptr<EventObserver>(&observer);
    if (observers_.find(observer_ptr) != observers_.end()) {
        return false;
    } else {
        observers_.insert(observer_ptr);
    }
    
    return true;
}

bool EventLoop::delObserver(EventObserver& observer) {
    std::shared_ptr<EventObserver> observer_ptr = std::shared_ptr<EventObserver>(&observer);
    if (observers_.find(observer_ptr) == observers_.end()) {
        return false;
    } else {
        observers_.erase(observer_ptr);
    }
    
    return true;
}

void EventLoop::notifyObserver(int step) {
    for (EventObserversSet::iterator ite = observers_.begin(); ite != observers_.end(); ite++) {
        std::shared_ptr<EventObserver> observer = *ite;
        if ((*observer).getStep() & step) {
            (*observer).arrive(*this);
        }
    }
}

long long EventLoop::findNextSourceTimeoutInms() {
    long long res = 0;
    
    for (EventSourceSet::iterator ite = sources_.begin(); ite != sources_.end(); ++ite) {
        std::shared_ptr<EventSource> source = *ite;
        if (res == 0) {
            res = source->getSourceData();
        } else {
            res = std::min(res, source->getSourceData());
        }
    }
    return res;
}

void EventLoop::arrive(SOURCE_IDENT orginal_ident, SOURCE_IDENT split_ident, int mask) {
    arrival_queue_.push_back(std::make_pair(orginal_ident, std::make_pair(split_ident, mask)));
}

void EventLoop::clearArrivalQueue() {
    arrival_queue_.clear();
}

std::shared_ptr<EventSource> EventLoop::findSourceByArrivalEvent(ArrivalEvent event) {
    
    std::shared_ptr<EventSource> res = NULL;
    
    for (EventSourceSet::iterator ite = sources_.begin(); ite != sources_.end(); ++ite) {
        std::shared_ptr<EventSource> source = *ite;
        if (event.first == (*source).getIdent() && event.second.second == (*source).getMask()) {
            return source;
        }
    }
    
    return res;
}
