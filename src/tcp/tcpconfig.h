//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include <stdint.h>

static const int32_t kConnectTimeout = 10 * 1000;
static const uint32_t kConnectInterval = 4 * 1000;
static const uint32_t kMaxConnectingCount = 3;

static const uint64_t kWaitInterval = 10 * 60 * 1000;
static const uint64_t kPreWaitIntervalThershold = 15 * 60 * 1000;
static const uint64_t kMainLongInterval = 5 * 1000;
static const uint64_t kMainShortInterval = 2 * 1000;

