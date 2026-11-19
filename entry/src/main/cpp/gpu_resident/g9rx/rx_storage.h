#pragma once
#include "receipt_clock.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unistd.h>
namespace grs::g9rx {
struct alignas(64) RxProtocolPayload {
  std::array<std::array<uint32_t, 256>, 20> witnesses{};
  std::array<std::array<uint32_t, 64>, 4> metadata{};
  std::array<uint32_t, 256> scratch{};
  std::array<std::array<uint32_t, 16>, 16> journal{};
  std::array<uint32_t, 256> control{};
  std::array<std::array<uint32_t, 128>, 19> inputs{};
  std::array<uint32_t, 1664> bridge{};
};
struct RxSync {
  const uint64_t boot = (uint64_t(getpid()) << 32) ^ CheckedClockRead().ns;
  bool Bump(std::atomic<uint64_t> &value) {
    uint64_t old = value.load();
    while (old != UINT64_MAX)
      if (value.compare_exchange_weak(old, old + 1))
        return true;
    ownerActive.store(false);
    return false;
  }
  bool CancelStroke() { return Bump(strokeCancelGeneration); }
  std::mutex protocol;
  std::atomic<uintptr_t> ingressEnv{0};
  std::atomic_flag ingress = ATOMIC_FLAG_INIT;
  std::atomic<uint64_t> strokeCancelGeneration{0}, lifecycleInvalidation{0},
      ownerPulseNs{0};
  std::atomic<bool> ownerActive{false}, ownerRootLive{false},
      mappingPending{false};
};
static_assert(sizeof(RxProtocolPayload) == 40960 &&
              alignof(RxProtocolPayload) == 64);
static_assert(offsetof(RxProtocolPayload, witnesses) == 0 &&
              offsetof(RxProtocolPayload, metadata) == 20480 &&
              offsetof(RxProtocolPayload, scratch) == 21504 &&
              offsetof(RxProtocolPayload, journal) == 22528 &&
              offsetof(RxProtocolPayload, control) == 23552 &&
              offsetof(RxProtocolPayload, inputs) == 24576 &&
              offsetof(RxProtocolPayload, bridge) == 34304);
static_assert(sizeof(RxSync) <= 256);
} // namespace grs::g9rx
