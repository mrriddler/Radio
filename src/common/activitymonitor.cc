//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "activitymonitor.h"

ActivityMonitor& ActivityMonitor::singleton() {
    static ActivityMonitor monitor;
    return monitor;
}

bool ActivityMonitor::isForeground() const {
    return false;
}
