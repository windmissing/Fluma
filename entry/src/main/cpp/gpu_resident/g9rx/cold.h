#pragma once
#include "cold_generated.h"
#include "controller.h"
namespace grs::g9rx {
class Cold {
  struct Slot {
    coldbroker::Lease credit;
    std::unique_ptr<uint32_t[]> block;
    std::array<uint32_t, 256> journal{};
    void Free() {
      block.reset();
      credit.Reset();
    }
    uint64_t request = 0, trace = 0, created = 0;
    size_t words = 0;
    bool pending = false, building = false, full = false;
  };
  std::mutex mutex_;
  std::array<Slot, 2> slots_{};
  void Expire(uint64_t now) {
    for (auto &s : slots_)
      if (s.block && !s.building && now >= s.created &&
          now - s.created >= 60000000000ULL) {
        s.Free();
        s = Slot{};
      }
  }

public:
  bool HasLive(CheckedTime time) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!time.ok)
      return true;
    Expire(time.ns);
    for (const auto &slot : slots_)
      if (slot.block)
        return true;
    return false;
  }
  std::string Begin(const Controller &, uint64_t replace, bool full,
                    CheckedTime);
  void Build(Controller &, CheckedTime);
  std::string State(uint64_t, CheckedTime);
  std::string Page(uint64_t, uint32_t, CheckedTime);
  std::string End(uint64_t, CheckedTime);
  void Reset();
};
} // namespace grs::g9rx
