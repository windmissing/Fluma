#pragma once
#define NOMINMAX
#include <Windows.h>
inline long syscall(long) { return static_cast<long>(GetCurrentThreadId()); }
