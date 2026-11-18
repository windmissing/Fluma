// T575 (problem T553 §2): SourceClaimWitnessV1 — a narrow, fixed-size witness
// of the source claim chain (enqueue -> G1 claim -> completion), locked to a
// designated enqueueSeq. It observes only; it never changes the consumption
// algorithm, wakes the owner, or extends capture age. Budget: witness struct
// <= 256B, ledger JSON <= 2048B, riding the existing 2MiB writer / 4MiB total
// diagnostic credit with complete-INCOMPLETE on overflow.
#pragma once
#include "../cold_protocol.h"
#include <array>
#include <cstring>
namespace grs::claim575 {
struct Witness {
 diagnostic492::Scope scope{};
 uint64_t intentSeq=0;       // caller intent sequence (raw words[6] family)
 uint64_t enqueueSeq=0;      // monotonic enqueue ordinal this attempt
 uint64_t enqueueNs=0;       // app-stamp at ENQUEUED_NOT_ACK
 uint64_t claimNs=0;         // 0 until the queue head is actually claimed
 uint32_t queueCount=0;      // queue depth at enqueue
 uint32_t headAge=0;         // ms from head enqueue to now, at claim
 uint64_t sourceGeneration=0;// source control boundary at enqueue
 uint64_t inputEpoch=0;      // input generation at enqueue
 uint64_t planId=0;          // run plan at claim (0 = not yet)
 uint64_t rootNonce=0;       // root nonce after claim (0 = none yet)
 uint32_t rootKind=0;        // 0 unknown, 1 source, 2 quantum
 uint32_t eligibilityBits=0; // bit0 allowed, bit1 fresh UI, bit2 queue nonempty, bit3 same epoch, bit4 active run
 uint64_t sourceCommit=0;    // sourceCommit counter observed with the claim
 uint32_t stage=0,terminalCode=0;char reason[32]{};          // first named no-claim reason when applicable
};
static_assert(sizeof(Witness)<=256,"T575 witness must stay within 256 bytes");
class Ledger {
 std::array<Witness,1> rows_{};size_t count_=0,dropped_=0;uint64_t nextEnqueue_=1;
 uint64_t lockedSeq_=0; // designated enqueueSeq for the narrow chain (0 = first)
public:
 void designate(uint64_t seq){lockedSeq_=seq;}
 // Called with the App mutex held at the enqueue point (ENQUEUED_NOT_ACK).
 Witness* onEnqueue(const diagnostic492::Point& point,uint64_t intentSeq,uint32_t queueCount,uint64_t sourceGeneration,uint64_t inputEpoch){
  if(nextEnqueue_==UINT64_MAX){++dropped_;return nullptr;}if(count_==rows_.size()){++nextEnqueue_;++dropped_;return nullptr;}
  if(lockedSeq_&&nextEnqueue_!=lockedSeq_){++nextEnqueue_;return nullptr;}
  auto& w=rows_[count_++];
  w={};w.scope=point.scope;w.intentSeq=intentSeq;w.enqueueSeq=nextEnqueue_++;w.enqueueNs=point.ns;
  w.queueCount=queueCount;w.sourceGeneration=sourceGeneration;w.inputEpoch=inputEpoch;
  w.stage=1;w.eligibilityBits=0x4u; // queue nonempty (just enqueued)
  return &w;
 }
 // Called with the App mutex held at the claim point (head consumed).
 void onClaim(uint64_t enqueueSeq,const diagnostic492::Point& point,uint32_t headAgeMs,uint64_t planId,uint64_t rootNonce,uint32_t rootKind,uint32_t eligibilityBits,uint64_t sourceCommit,const char* reason){
  for(auto& w:rows_)if(w.enqueueSeq==enqueueSeq&&std::memcmp(&w.scope,&point.scope,sizeof(w.scope))==0){
   w.stage=4;w.claimNs=point.ns;w.headAge=headAgeMs;w.planId=planId;w.rootNonce=rootNonce;w.rootKind=rootKind;
   w.eligibilityBits|=eligibilityBits;w.sourceCommit=sourceCommit;
   const size_t n=std::strlen(reason);std::memcpy(w.reason,reason,n<sizeof(w.reason)-1?n:sizeof(w.reason)-1);
   return;
  }
 }
 uint64_t Sequence(uint64_t intent,const diagnostic492::Scope& scope)const{for(const auto& w:rows_)if(w.stage<6&&w.intentSeq==intent&&!std::memcmp(&w.scope,&scope,sizeof(scope)))return w.enqueueSeq;return 0;}
 void NoClaim(const diagnostic492::Scope& scope,const char* reason){for(auto& w:rows_)if(w.enqueueNs&&w.stage<4&&!w.reason[0]&&!std::memcmp(&w.scope,&scope,sizeof(scope))){std::strncpy(w.reason,reason,sizeof(w.reason)-1);}}
 void Attempt(uint64_t seq,const diagnostic492::Scope& scope){for(auto& w:rows_)if(w.enqueueSeq==seq&&seq&&!std::memcmp(&w.scope,&scope,sizeof(scope)))w.stage=3;}
 void Observe(uint64_t intent,const diagnostic492::Point& p,uint32_t stage,uint64_t plan,uint64_t nonce,uint64_t commit,uint32_t code){for(auto& w:rows_)if(w.intentSeq==intent&&!std::memcmp(&w.scope,&p.scope,sizeof(w.scope))&&w.stage>=4&&w.stage<6){w.stage=stage;w.planId=plan;w.rootNonce=nonce;w.rootKind=1;w.sourceCommit=commit;w.terminalCode=code;}}
 void noteEligibility(uint64_t enqueueSeq,uint32_t bits){
  for(auto& w:rows_)if(w.enqueueSeq==enqueueSeq&&enqueueSeq&&w.stage<6){w.eligibilityBits|=bits;if(w.stage<2)w.stage=2;}
 }
 void Json(cold503::Writer& w)const{
  const size_t begin=w.Size();
  w.Raw("{\"revision\":\"SourceClaimWitnessV1\",\"witnessBytes\":");
  w.Number(sizeof(Witness));w.Raw(",\"designatedEnqueueSeq\":");w.U64(lockedSeq_);
  w.Raw(",\"dropped\":");w.Number(dropped_);w.Raw(",\"rows\":[");
  bool comma=false;
  for(const auto& r:rows_){if(r.enqueueNs==0)continue;if(comma)w.Raw(",");comma=true;
   w.Raw("{\"scope\":");cold503::ControlJson(w,r.scope);
   w.Raw(",\"intentSeq\":");w.U64(r.intentSeq);
   w.Raw(",\"enqueueSeq\":");w.U64(r.enqueueSeq);
   w.Raw(",\"enqueueNs\":");w.U64(r.enqueueNs);
   w.Raw(",\"stage\":");w.Number(r.stage);w.Raw(",\"terminalCode\":");w.Number(r.terminalCode);w.Raw(",\"claimNs\":");w.U64(r.claimNs);
   w.Raw(",\"queueCount\":");w.Number(r.queueCount);
   w.Raw(",\"headAgeMs\":");w.Number(r.headAge);
   w.Raw(",\"sourceGeneration\":");w.U64(r.sourceGeneration);
   w.Raw(",\"inputEpoch\":");w.U64(r.inputEpoch);
   w.Raw(",\"planId\":");w.U64(r.planId);
   w.Raw(",\"rootNonce\":");w.U64(r.rootNonce);
   w.Raw(",\"rootKind\":");w.Number(r.rootKind);
   w.Raw(",\"eligibilityBits\":");w.Number(r.eligibilityBits);
   w.Raw(",\"sourceCommit\":");w.U64(r.sourceCommit);
   w.Raw(",\"reason\":");w.String(r.reason,std::strlen(r.reason));
   w.Raw("}");}
  w.Raw("]}");if(w.Size()-begin>2048)w.Append(nullptr,SIZE_MAX);
 }
};
static_assert(sizeof(Ledger)<=256,"T597 single designated witness credit");
}
