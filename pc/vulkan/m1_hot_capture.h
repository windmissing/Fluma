#pragma once
#include "m1_ingress.h"
#include <optional>
#include <algorithm>
#include <string>
#include <vector>

namespace grs::g13vk {
class M1HotCapture {
 public:
  enum class Phase { Idle, SourceFence, QuantumFence, GraphicsFence, Present };
  enum class Code { Idle, NotBusy, Reserved, Conflict, Cancelled, TimedOut, Frozen, Read, Ended,
                    Copying, ColdFrozen, ColdRead, Failed };
  static constexpr size_t kColdLimit=16u*1024u*1024u;
  static constexpr size_t kPageLimit=64u*1024u;
  struct Page {
    SourceScope scope;
    uint64_t request=0, requestedNs=0, busyRoot=0, safePoint=0, frozenNs=0;
    Phase blocker=Phase::Idle;
    std::array<uint32_t,256> completion{};
  };
  struct ColdPage {
    Page lineage;
    std::string sha;
    size_t offset=0, count=0, total=0;
  };

  void Busy(Phase phase, uint64_t nonce) {
    std::lock_guard lock(mutex_);
    phase_=phase;
    nonce_=nonce;
  }
  void StopBusy() {
    std::lock_guard lock(mutex_);
    phase_=Phase::Idle;
  }
  Code Reserve(const SourceScope &scope,uint64_t request,uint64_t now) {
    std::lock_guard lock(mutex_);
    Expire(now);
    if (!request || !(scope==scope_)) return Code::Cancelled;
    if (request==page_.request) return code_;
    if (request<page_.request) return Code::Cancelled;
    if (IsLive()) return Code::Conflict;
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
    if (IsLive() && !(scope==page_.scope)) Retire(Code::Cancelled);
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
    if (scope==scope_ && request==page_.request && code_==Code::ColdRead) Retire(Code::Ended);
  }
  std::optional<Page> ClaimCold(uint64_t nonce,uint64_t now) {
    std::lock_guard lock(mutex_);
    Expire(now);
    if (code_!=Code::Frozen || nonce!=page_.busyRoot || page_.blocker!=Phase::QuantumFence) return {};
    if (now>=page_.requestedNs && now-page_.requestedNs>=1000000000ull) {
      Retire(Code::TimedOut);
      return {};
    }
    code_=Code::Copying;
    return page_;
  }
  bool SealCold(const Page &token,std::vector<uint8_t> &bytes,const std::string &sha,uint64_t now) {
    std::lock_guard lock(mutex_);
    if (code_!=Code::Copying || !(scope_==token.scope) || page_.request!=token.request ||
        page_.busyRoot!=token.busyRoot) return false;
    if (bytes.empty() || bytes.size()>kColdLimit || sha.size()!=64) {
      Retire(Code::Failed);
      return false;
    }
    cold_=std::move(bytes);
    sha_=sha;
    page_.frozenNs=now;
    next_=0;
    code_=Code::ColdFrozen;
    return true;
  }
  void FailCold(const Page &token) {
    std::lock_guard lock(mutex_);
    if (code_==Code::Copying && scope_==token.scope && page_.request==token.request)
      Retire(Code::Failed);
  }
  std::optional<ColdPage> ReadCold(const SourceScope &scope,uint64_t request,size_t offset,
                                   uint8_t *out,size_t capacity,uint64_t now) {
    std::lock_guard lock(mutex_);
    Expire(now);
    if (!(scope==scope_) || request!=page_.request || (code_!=Code::ColdFrozen && code_!=Code::ColdRead) ||
        offset!=next_ || offset>=cold_.size() || !out || !capacity || capacity>kPageLimit) return {};
    const size_t count=std::min(capacity,cold_.size()-offset);
    std::copy_n(cold_.data()+offset,count,out);
    next_+=count;
    code_=Code::ColdRead;
    return ColdPage{page_,sha_,offset,count,cold_.size()};
  }
 private:
  bool IsLive() const {
    return code_==Code::Reserved || code_==Code::Frozen || code_==Code::Read ||
           code_==Code::Copying || code_==Code::ColdFrozen || code_==Code::ColdRead;
  }
  void Retire(Code code) {
    std::vector<uint8_t>().swap(cold_);
    sha_.clear();
    next_=0;
    code_=code;
  }
  void Expire(uint64_t now) {
    if (code_==Code::Reserved && now>=page_.requestedNs && now-page_.requestedNs>=1000000000ull)
      code_=Code::TimedOut;
    if ((code_==Code::Frozen || code_==Code::Read || code_==Code::ColdFrozen || code_==Code::ColdRead) &&
        now>=page_.frozenNs && now-page_.frozenNs>=60000000000ull) Retire(Code::Ended);
  }
  std::mutex mutex_;
  SourceScope scope_;
  Phase phase_=Phase::Idle;
  Code code_=Code::Idle;
  uint64_t nonce_=0, safe_=0;
  Page page_;
  size_t next_=0;
  std::vector<uint8_t> cold_;
  std::string sha_;
};
} // namespace grs::g13vk
