//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "time.h"

#include <mach/mach_time.h>

uint64_t gettickcount() {
    static mach_timebase_info_data_t timebase_info = {0};
    
    // Convert to nanoseconds - if this is the first time we've run, get the timebase.
    if (timebase_info.denom == 0 )
    {
        (void) mach_timebase_info(&timebase_info);
    }
    
    // Convert the mach time to milliseconds
    uint64_t mach_time = mach_absolute_time();
    uint64_t millis = (mach_time * timebase_info.numer) / (timebase_info.denom * 1000000);
    return millis;
}
