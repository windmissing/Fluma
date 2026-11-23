#pragma once
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace grs::g13vk {
struct M1D1Intake {
  static constexpr uint64_t kDrainNs=1000000000;
  struct End { uint64_t ns=0, pulse=0, offer=0; };
  void Start() {
    std::lock_guard lock(mutex_);
    started_=true;
  }
  void Seal(End end) {
    std::lock_guard lock(mutex_);
    if (!end_.ns) end_=end;
    changed_.notify_all();
  }
  End Snapshot() {
    std::lock_guard lock(mutex_);
    return end_;
  }
  End Await() {
    std::unique_lock lock(mutex_);
    if (started_) changed_.wait_for(lock,std::chrono::nanoseconds(kDrainNs),[&] { return end_.ns!=0; });
    return end_;
  }
  bool MarkReported() {
    std::lock_guard lock(mutex_);
    if (reported_) return false;
    reported_=true;
    return true;
  }
 private:
  std::mutex mutex_;
  std::condition_variable changed_;
  End end_{};
  bool started_=false,reported_=false;
};
}
