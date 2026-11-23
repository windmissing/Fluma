#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <mutex>
#include <utility>
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
  enum class Outcome { Accepted, Rejected, Revoked };
  struct Terminal {
    M1PulseScope scope;
    M1Pulse pulse;
    const char *reason;
    Outcome outcome;
    uint64_t qpc=0,generated=0,carry=0,overload=0;
    uint32_t pending=0;
    bool clockObserved=false;
  };
  struct Receipt { M1Pulse pulse; const char *state; };
  struct SessionCoverage {
    M1PulseScope scope;
    uint64_t queued=0,accepted=0,rejected=0,revoked=0;
  };
  struct Coverage {
    std::array<SessionCoverage,64> sessions{};
    size_t size=0;
    uint64_t lost=0;
    bool Complete() const {
      if (lost) return false;
      for (size_t i=0;i<size;++i) {
        const auto &s=sessions[i];
        if ((s.accepted|s.rejected|s.revoked)!=s.queued) return false;
      }
      return true;
    }
  };
  // Configure once before the producer and owner start.
  void Observe(std::function<uint64_t()> ticks,std::function<void(const Terminal &)> terminal) {
    ticks_=std::move(ticks);
    terminal_=std::move(terminal);
  }
  void Begin(M1PulseScope scope,uint64_t afterTicks) {
    Replace(scope,afterTicks,"REVOKED_REANCHOR");
  }
  void Close() { Replace({},0,"REVOKED_CLOSED"); }

  M1Pulse Receive(uint64_t geometry,M1Pulse pulse) {
    return ReceiveWithReceipt(geometry,pulse).pulse;
  }
  Receipt ReceiveWithReceipt(uint64_t geometry,M1Pulse pulse) {
    std::lock_guard lock(mutex_);
    if (!scope_.session) return {{},"PULSE_NO_SESSION"};
    if (geometry!=scope_.geometry) return {{},"PULSE_STALE_GEOMETRY"};
    if (pulse.callbackTicks<=after_) return {{},"PULSE_BEFORE_BASELINE"};
    if (rejected_) return {{},rejected_};
    if (size_==rows_.size()) {
      rejected_="PULSE_QUEUE_OVERFLOW";
      return {{},rejected_};
    }
    pulse.seq=latest_.seq+1;
    pulse.session=scope_.session;
    if (!pulse.seq || pulse.receiptTicks<pulse.callbackTicks || pulse.receiptNs<pulse.uiNs) {
      rejected_="PULSE_RECEIPT_INVALID";
      return {{},rejected_};
    }
    latest_=pulse;
    rows_[size_++]=pulse;
    if (terminal_) {
      auto *s=Session(scope_);
      if (!s || pulse.seq>64) ++coverage_.lost;
      else s->queued|=uint64_t{1}<<(pulse.seq-1);
    }
    return {pulse,"QUEUED"};
  }

  bool Consume(M1PulseScope scope,m1::ProgressClockV2 &clock,M1Pulse &latest,M1Pulse &accepted,
               const Observer &observe={}) {
    std::array<M1Pulse,kCapacity> batch;
    size_t count;
    const char *failure=nullptr;
    {
      std::lock_guard lock(mutex_);
      if (!(scope==scope_)) { clock.Revoke("STALE_PULSE_SCOPE"); return false; }
      latest=latest_;
      failure=rejected_;
      count=size_;
      std::copy_n(rows_.begin(),count,batch.begin());
      size_=0;
    }
    if (failure) clock.Revoke(failure);
    for (size_t i=0;i<count;++i) {
      const auto &pulse=batch[i];
      Terminal entry;
      {
        std::lock_guard lock(mutex_);
        const bool stale=!(scope==scope_);
        const char *reason=nullptr;
        auto outcome=Outcome::Revoked;
        if (stale) reason=scope_.session ? "REVOKED_REANCHOR" : "REVOKED_CLOSED";
        else if (failure) reason=!std::strcmp(failure,"PULSE_QUEUE_OVERFLOW") ?
                                 "REVOKED_OVERFLOW" : "PULSE_REVOKED_AFTER_CLOCK_FAULT";
        else {
          const auto status=pulse.seq==clock.sequence+1 ?
              clock.Pulse(scope.session,pulse.seq,pulse.uiNs,pulse.receiptNs) : "PULSE_SEQUENCE_GAP";
          reason=status;
          outcome=!std::strcmp(status,"PAGE_PULSE_ACCEPTED") ? Outcome::Accepted : Outcome::Rejected;
          if (outcome==Outcome::Accepted) accepted=pulse;
          else {
            if (clock.open) clock.Revoke(status);
            failure=status;
          }
        }
        entry=FinishLocked(scope,pulse,reason,outcome,outcome==Outcome::Revoked ? nullptr : &clock);
        if (stale && !failure) { failure="STALE_PULSE_SCOPE"; clock.Revoke(failure); }
      }
      if (terminal_) terminal_(entry);
      if (observe) observe(pulse,entry.reason);
    }
    return !failure && clock.open;
  }
  void RevokeQueued(M1PulseScope scope,const Observer &observe) {
    std::array<Terminal,kCapacity> batch;
    size_t count=0;
    {
      std::lock_guard lock(mutex_);
      if (!(scope==scope_)) return;
      count=ClearLocked(batch,"END_INTAKE_REVOKED");
    }
    for (size_t i=0;i<count;++i) {
      if (terminal_) terminal_(batch[i]);
      if (observe) observe(batch[i].pulse,batch[i].reason);
    }
  }
  Coverage Ledger(uint64_t run,uint64_t geometry) {
    std::lock_guard lock(mutex_);
    Coverage result;
    result.lost=coverage_.lost;
    for (size_t i=0;i<coverage_.size;++i) {
      const auto &s=coverage_.sessions[i];
      if (s.scope.run==run && s.scope.geometry==geometry) result.sessions[result.size++]=s;
    }
    return result;
  }
 private:
  SessionCoverage *Session(M1PulseScope scope) {
    for (size_t i=0;i<coverage_.size;++i)
      if (coverage_.sessions[i].scope==scope) return &coverage_.sessions[i];
    if (coverage_.size==coverage_.sessions.size()) return nullptr;
    auto &s=coverage_.sessions[coverage_.size++];
    s.scope=scope;
    return &s;
  }
  Terminal FinishLocked(M1PulseScope scope,const M1Pulse &pulse,const char *reason,Outcome outcome,
                        const m1::ProgressClockV2 *clock=nullptr) {
    Terminal entry{scope,pulse,reason,outcome};
    if (!terminal_) return entry;
    entry.qpc=ticks_ ? ticks_() : 0;
    if (clock) {
      entry.clockObserved=true;
      entry.generated=clock->generated;
      entry.pending=clock->pending;
      entry.carry=clock->carry;
      entry.overload=clock->overload;
    }
    auto *s=Session(scope);
    if (!s || !pulse.seq || pulse.seq>64) { ++coverage_.lost; return entry; }
    const auto bit=uint64_t{1}<<(pulse.seq-1);
    if (!(s->queued&bit) || ((s->accepted|s->rejected|s->revoked)&bit)) {
      ++coverage_.lost;
    } else {
      auto &dst=outcome==Outcome::Accepted ? s->accepted :
          outcome==Outcome::Rejected ? s->rejected : s->revoked;
      dst|=bit;
    }
    return entry;
  }
  size_t ClearLocked(std::array<Terminal,kCapacity> &batch,const char *reason) {
    const auto count=size_;
    for (size_t i=0;i<count;++i) batch[i]=FinishLocked(scope_,rows_[i],reason,Outcome::Revoked);
    size_=0;
    return count;
  }
  void Replace(M1PulseScope scope,uint64_t afterTicks,const char *reason) {
    std::array<Terminal,kCapacity> batch;
    size_t count;
    {
      std::lock_guard lock(mutex_);
      count=ClearLocked(batch,reason);
      scope_=scope;
      after_=afterTicks;
      latest_={};
      rejected_=nullptr;
    }
    if (terminal_) for (size_t i=0;i<count;++i) terminal_(batch[i]);
  }
  std::mutex mutex_;
  M1PulseScope scope_{};
  uint64_t after_=0;
  size_t size_=0;
  M1Pulse latest_{};
  const char *rejected_=nullptr;
  std::array<M1Pulse,kCapacity> rows_{};
  std::function<uint64_t()> ticks_;
  std::function<void(const Terminal &)> terminal_;
  Coverage coverage_;
};
}
