#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <mutex>

namespace grs::g13vk {
struct PointerSample {
  uint64_t hold=0, sequence=0, timeNs=0;
  double x=0, y=0;
  int32_t clientX=0, clientY=0;
  uint32_t material=0;
};

struct SourceScope {
  uint64_t run=0, owner=0, context=0, surface=0, geometry=0, config=0, permission=0, pageSession=0;
  bool operator==(const SourceScope &b) const {
    return run==b.run && owner==b.owner && context==b.context && surface==b.surface &&
           geometry==b.geometry && config==b.config && permission==b.permission && pageSession==b.pageSession;
  }
};

struct SourceOffer {
  SourceScope scope;
  uint64_t seq=0, pulseNs=0, pulseTicks=0, sampleTicks=0, frequency=0;
  PointerSample sample;
  uint32_t origin=0;
  uint32_t action=0;
  uint64_t deltaNs=0;
  bool operator==(const SourceOffer &b) const {
    const auto &a=sample, &s=b.sample;
    return scope==b.scope && seq==b.seq && pulseNs==b.pulseNs && pulseTicks==b.pulseTicks &&
           sampleTicks==b.sampleTicks && frequency==b.frequency && origin==b.origin &&
           action==b.action && deltaNs==b.deltaNs &&
           a.hold==s.hold && a.sequence==s.sequence && a.timeNs==s.timeNs &&
           a.x==s.x && a.y==s.y && a.clientX==s.clientX && a.clientY==s.clientY &&
           a.material==s.material;
  }
};

class M1Ingress {
 public:
  enum class Code { Pending, Consuming, Accepted, Rejected, Revoked, Full, Conflict, Expired, Invalid };
  struct Result { Code code=Code::Invalid; uint64_t root=0; };
  struct Trace { uint64_t serial=0; SourceOffer offer; Result result; bool terminal=false; };

  void EnableTrace() { traceEnabled_=true; } // Before either thread starts.
  std::size_t PendingCount() {
    std::lock_guard lock(mutex_);
    std::size_t count=0;
    for (const auto &slot:active_) count+=slot.offer.seq!=0;
    return count;
  }
  std::size_t ReadTrace(Trace *out,std::size_t capacity,uint64_t &dropped) {
    std::lock_guard lock(mutex_);
    std::size_t count=0;
    while (used_ && count<capacity) {
      out[count++]=trace_[read_];
      read_=(read_+1)%trace_.size();
      --used_;
    }
    dropped=dropped_;
    return count;
  }

  void Authorize(const SourceScope &scope, uint64_t hold) {
    std::lock_guard lock(mutex_);
    if (scope==scope_ && hold==hold_) return;
    for (auto &slot : active_) if (slot.offer.seq) Archive(slot, {Code::Revoked});
    if (scope.run!=scope_.run || scope.owner!=scope_.owner || scope.context!=scope_.context) {
      recent_={}; cursor_=0; last_=0;
    }
    scope_=scope;
    hold_=hold;
  }

  Result OfferSource(const SourceOffer &offer) {
    std::lock_guard lock(mutex_);
    const auto report=[&](Result result) {
      Record(offer,result,false);
      return result;
    };
    if (!offer.seq) return report({Code::Invalid});
    for (const auto &group : {&active_, &recent_})
      for (const auto &slot : *group)
        if (slot.offer.seq==offer.seq)
          return report(slot.offer==offer ? slot.result : Result{Code::Conflict});
    if (!(offer.scope==scope_) || !hold_ || offer.sample.hold!=hold_)
      return report({Code::Revoked});
    if (offer.seq<=last_) return report({Code::Expired});
    const auto &sample=offer.sample;
    if ((offer.action && (!offer.deltaNs || offer.deltaNs>33333333)) ||
        sample.material>1 || !std::isfinite(sample.x) || !std::isfinite(sample.y) ||
        sample.x<0 || sample.x>=128 || sample.y<0 || sample.y>256 ||
        !offer.frequency || !sample.sequence || !offer.sampleTicks ||
        offer.pulseTicks<=offer.sampleTicks || offer.pulseNs<=offer.sample.timeNs)
      return report({Code::Invalid});
    for (auto &slot : active_) if (!slot.offer.seq) {
      last_=offer.seq;
      slot={offer, {Code::Pending}};
      return report(slot.result);
    }
    return report({Code::Full});
  }

  SourceOffer Take() {
    std::lock_guard lock(mutex_);
    Slot *first=nullptr;
    for (auto &slot : active_)
      if (slot.offer.seq && slot.result.code==Code::Pending && (!first || slot.offer.seq<first->offer.seq))
        first=&slot;
    if (!first) return {};
    first->result={Code::Consuming};
    return first->offer;
  }

  void Finish(uint64_t seq, Result result) {
    std::lock_guard lock(mutex_);
    for (auto &slot : active_) if (slot.offer.seq==seq && slot.result.code==Code::Consuming) {
      Archive(slot, result);
      break;
    }
  }

 private:
  struct Slot { SourceOffer offer; Result result; };
  void Record(const SourceOffer &offer,Result result,bool terminal) {
    if (!traceEnabled_) return;
    ++traceSerial_;
    if (used_==trace_.size()) { ++dropped_; return; }
    trace_[(read_+used_)%trace_.size()]={traceSerial_,offer,result,terminal};
    ++used_;
  }
  void Archive(Slot &slot, Result result) {
    Record(slot.offer,result,true);
    recent_[cursor_]={slot.offer, result};
    cursor_=(cursor_+1)%recent_.size();
    slot={};
  }
  std::mutex mutex_;
  SourceScope scope_;
  uint64_t hold_=0, last_=0;
  std::size_t cursor_=0;
  std::array<Slot, 4> active_{}, recent_{};
  bool traceEnabled_=false;
  uint64_t traceSerial_=0, dropped_=0;
  std::size_t read_=0, used_=0;
  std::array<Trace,256> trace_{};
};
} // namespace grs::g13vk
