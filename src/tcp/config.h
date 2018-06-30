//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include <stdint.h>

/*
 *  race connection
 */
static const uint64_t kRaceConnectInterval = 10 * 1000;
static const uint64_t kRaceConnectStep = 4 * 1000;

static const uint32_t kMaxConnectingCount = 3;

/*
 *  smart heart beat
 */
static const uint64_t kMaxHeartInterval = 9 * 60 * 1000 + 50 * 1000;
static const uint64_t kMinHeartInterval = 4  * 60 * 1000 + 30 * 1000;

static const uint64_t kHeartStep = 60 * 1000;
static const uint64_t kHeartAvoidMaxStep = 20 * 1000;

static const uint32_t kHeartProbeCount = 3;
static const uint32_t kMaxHeartContinuousSuccessCount = 3;
static const uint32_t kMaxHeartContinuousFailureCount = 3;
