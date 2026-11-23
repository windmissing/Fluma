#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <mutex>
#include "../../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h"

namespace grs::g13vk {
struct M1PulseScope {
  uint64_t run=0, geometry=0, session=0;
  bool operator==(const M1PulseScope &other) const {
    return run==other.run && geometry==other.geometry && session==other.session;
  }
};
struct M1Pulse {
  uint64_t seq=0, callbackTicks=0, receiptTicks=0, uiNs=0, receiptNs=0;
  uint64_t session=0;
  uint32_t action=0;
};

class M1PulseQueue {
 public:
  using Observer=std::function<void(const M1Pulse &,const char *)>;
  static constexpr size_t kCapacity=32;
  void Begin(M1PulseScope scope,uint64_t afterTicks) {
    std::lock_guard lock(mutex_);
    scope_=scope;
    after_=afterTicks;
    size_=0;
    latest_={};
    rejected_=nullptr;
  }
  void Close() { Begin({},0); }

  M1Pulse Receive(uint64_t geometry,M1Pulse pulse) {
    std::lock_guard lock(mutex_);
    if (!scope_.session || geometry!=scope_.geometry || pulse.callbackTicks<=after_) return {};
    pulse.seq=latest_.seq+1;
    pulse.session=scope_.session;
    latest_=pulse;
    if (rejected_) return {};
    if (size_==rows_.size()) { rejected_="PULSE_QUEUE_OVERFLOW"; return {}; }
    if (!pulse.seq || pulse.receiptTicks<pulse.callbackTicks || pulse.receiptNs<pulse.uiNs) {
      rejected_="PULSE_RECEIPT_INVALID";
      return {};
    }
    rows_[size_++]=pulse;
    return pulse;
  }

  bool Consume(M1PulseScope scope,m1::ProgressClockV2 &clock,M1Pulse &latest,M1Pulse &accepted,
               const Observer &observe={}) {
    std::array<M1Pulse,kCapacity> batch;
    size_t count;
    {
      std::lock_guard lock(mutex_);
      if (!(scope==scope_)) { clock.Revoke("STALE_PULSE_SCOPE"); return false; }
      latest=latest_;
      if (rejected_) { clock.Revoke(rejected_); return false; }
      count=size_;
      std::copy_n(rows_.begin(),count,batch.begin());
      size_=0;
    }
    for (size_t i=0;i<count;++i) {
      const auto &pulse=batch[i];
      if (pulse.seq!=clock.sequence+1) {
        if (observe) for (;i<count;++i) observe(batch[i],"PULSE_SEQUENCE_GAP");
        clock.Revoke("PULSE_SEQUENCE_GAP");
        return false;
      }
      const auto status=clock.Pulse(scope.session,pulse.seq,pulse.uiNs,pulse.receiptNs);
      if (observe) observe(pulse,status);
      if (std::strcmp(status,"PAGE_PULSE_ACCEPTED")!=0) {
        if (observe) for (++i;i<count;++i) observe(batch[i],"PULSE_REVOKED_AFTER_CLOCK_FAULT");
        if (clock.open) clock.Revoke(status);
        return false;
      }
      accepted=pulse;
    }
    return clock.open;
  }
  void RevokeQueued(M1PulseScope scope,const Observer &observe) {
    std::array<M1Pulse,kCapacity> batch;
    size_t count;
    {
      std::lock_guard lock(mutex_);
      if (!(scope==scope_)) return;
      count=size_;
      std::copy_n(rows_.begin(),count,batch.begin());
      size_=0;
    }
    if (observe) for (size_t i=0;i<count;++i) observe(batch[i],"END_INTAKE_REVOKED");
  }
 private:
  std::mutex mutex_;
  M1PulseScope scope_{};
  uint64_t after_=0;
  size_t size_=0;
  M1Pulse latest_{};
  const char *rejected_=nullptr;
  std::array<M1Pulse,kCapacity> rows_{};
};
}
