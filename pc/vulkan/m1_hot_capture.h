#pragma once
#include "m1_ingress.h"
#include <optional>

namespace grs::g13vk {
class M1HotCapture {
 public:
  enum class Phase { Idle, SourceFence, QuantumFence };
  enum class Code { Idle, NotBusy, Reserved, Conflict, Cancelled, TimedOut, Frozen, Read, Ended };
  struct Page {
    SourceScope scope;
    uint64_t request=0, requestedNs=0, busyRoot=0, safePoint=0, frozenNs=0;
    Phase blocker=Phase::Idle;
    std::array<uint32_t,256> completion{};
  };

  void Busy(Phase phase, uint64_t nonce) {
    std::lock_guard lock(mutex_);
    phase_=phase;
    nonce_=nonce;
  }
  Code Reserve(const SourceScope &scope,uint64_t request,uint64_t now) {
    std::lock_guard lock(mutex_);
    Expire(now);
    if (!request || !(scope==scope_)) return Code::Cancelled;
    if (request==page_.request) return code_;
    if (request<page_.request) return Code::Cancelled;
    if (code_==Code::Reserved || code_==Code::Frozen || code_==Code::Read) return Code::Conflict;
    if (phase_==Phase::Idle) return Code::NotBusy;
    page_={scope,request,now,nonce_,0,0,phase_};
    return code_=Code::Reserved;
  }
  void Safe(uint64_t nonce,const std::array<uint32_t,256> *accepted,uint64_t now) {
    std::lock_guard lock(mutex_);
    phase_=Phase::Idle;
    ++safe_;
    Expire(now);
    if (code_!=Code::Reserved) return;
    if (!accepted || nonce!=page_.busyRoot) {
      code_=Code::Cancelled;
      return;
    }
    page_.safePoint=safe_;
    page_.frozenNs=now;
    page_.completion=*accepted;
    code_=Code::Frozen;
  }
  void Authorize(const SourceScope &scope) {
    std::lock_guard lock(mutex_);
    scope_=scope;
    if ((code_==Code::Reserved || code_==Code::Frozen || code_==Code::Read) && !(scope==page_.scope))
      code_=Code::Cancelled;
  }
  Code Cancel(const SourceScope &scope,uint64_t request) {
    std::lock_guard lock(mutex_);
    if (scope==scope_ && request && request==page_.request && code_==Code::Reserved)
      code_=Code::Cancelled;
    return code_;
  }
  std::optional<Page> Read(const SourceScope &scope,uint64_t request,uint64_t now) {
    std::lock_guard lock(mutex_);
    Expire(now);
    if (!(scope==scope_) || request!=page_.request || code_!=Code::Frozen) return {};
    code_=Code::Read;
    return page_;
  }
  void End(const SourceScope &scope,uint64_t request) {
    std::lock_guard lock(mutex_);
    if (scope==scope_ && request==page_.request && code_==Code::Read) code_=Code::Ended;
  }
 private:
  void Expire(uint64_t now) {
    if (code_==Code::Reserved && now>=page_.requestedNs && now-page_.requestedNs>=1000000000ull)
      code_=Code::TimedOut;
    if ((code_==Code::Frozen || code_==Code::Read) && now>=page_.frozenNs && now-page_.frozenNs>=60000000000ull)
      code_=Code::Ended;
  }
  std::mutex mutex_;
  SourceScope scope_;
  Phase phase_=Phase::Idle;
  Code code_=Code::Idle;
  uint64_t nonce_=0, safe_=0;
  Page page_;
};
} // namespace grs::g13vk
