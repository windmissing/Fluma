#pragma once
// T638: read-only route13 observation callbacks; no API calls or blocking owner
// work.
#include <cstddef>
#include <cstdint>
namespace grs::programcache {
struct ProgramObservationHook {
  void *context = nullptr;
  void (*stage)(void *, const char *, const char *, uint32_t, int64_t,
                const char *) = nullptr;
  void (*log)(void *, uint32_t, const char *, size_t, int64_t) = nullptr;
};
inline thread_local ProgramObservationHook route13Hook{};
inline thread_local ProgramObservationHook route5Hook{};
inline void Observe13(const char *phase, const char *edge, uint32_t object = 0,
                      int64_t raw = -1, const char *sha = nullptr) {
  if (route13Hook.stage)
    route13Hook.stage(route13Hook.context, phase, edge, object, raw, sha);
}
} // namespace grs::programcache
