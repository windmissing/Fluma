#pragma once
// Host-test-only shim. Product still uses CLOCK_MONOTONIC on OpenHarmony.
#include <chrono>
#include <ctime>
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
inline int clock_gettime(int, timespec* out) {
 auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
 out->tv_sec=static_cast<time_t>(ns/1000000000);out->tv_nsec=static_cast<long>(ns%1000000000);return 0;
}
#endif
#ifndef __clang__
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif
