#pragma once
#include "buffer_plan.h"
#include "quota_reader_v3.h"
#include "../cold_protocol.h"
namespace grs::m3a::cold {
inline constexpr const char* kRevision="GRS_M3_BOUNDED_COLD_V1";
inline constexpr size_t kPageBytes=16384,kPageWords=1024,kQuotaBytes=1024,kControlBytes=8192,kInputBytes=2176;
inline constexpr size_t kMaximumTraceBytes=27847808,kTwoSlotTraceBytes=2*kMaximumTraceBytes;
inline constexpr size_t kFullDescriptorWords=32+8192;
inline constexpr size_t kMaximumFullTraceBytes=kMaximumTraceBytes+kFullDescriptorWords*4+20*27006*256;
inline constexpr size_t kTwoSlotFullTraceBytes=2*kMaximumFullTraceBytes;
inline constexpr size_t kDiagnosticBytes=2097152;
struct Layout {
 size_t completion=0,state=256,quota=0,control=0,input=0,branchMeta=0,branchControl=0,branchEvents=0,events=0,words=0,residentPayload=0;
 bool fullBranch=false;
 static bool Make(const Profile& p,size_t alignment,Layout& out,bool full=false){
  ResidentBuffersV4 resident;if(!ResidentBuffersV4::Make(p,alignment,resident))return false;
  Layout n;n.quota=n.state+p.stateBytes/4;n.control=n.quota+256;n.input=n.control+2048;n.events=n.input+544;
  n.fullBranch=full;
  if(full){n.branchMeta=n.events;n.branchControl=n.branchMeta+32;n.branchEvents=n.branchControl+8192;n.events=n.branchEvents+20*size_t(p.scopeRows)*64;}
  n.words=n.events+std::max(size_t(p.quantumFrameRows)*64,size_t(p.sourceRows)*128);n.residentPayload=resident.total;
  if(full)n.residentPayload+=20*size_t(p.scopeRows)*256+32768;
  if(n.words>(full?kMaximumFullTraceBytes:kMaximumTraceBytes)/4)return false;out=n;return true;
 }
};
// T507 bounded writer reused as a utility, never its M2 format/route/budgets.
using Writer=cold503::Writer;
inline bool Quota(Writer& writer,const QuotaSnapshotV3& quota){
 if(InspectQuotaSnapshotV3(quota).result!=ControlResult::OK)return false;
 if(!writer.Raw("{\"revision\":\"GRS_M3_QUOTA_SNAPSHOT_V3\",\"words\":["))return false;
 for(size_t i=0;i<quota.size();++i){if(i&&!writer.Raw(","))return false;if(!writer.Number(quota[i]))return false;}
 return writer.Raw("]}");
}
inline bool Capabilities(Writer& w,const Profile& p,size_t alignment,bool full=false){
 Layout l;if(!Layout::Make(p,alignment,l,full))return false;
 return w.Raw("{\"revision\":\"GRS_M3_BOUNDED_COLD_V1\",\"route\":\"m3a\",\"wire\":4,\"features\":15,\"quotaRevision\":3,\"residentPayloadBytes\":")&&w.U64(l.residentPayload)&&
 w.Raw(",\"traceBytes\":")&&w.U64(l.words*4)&&w.Raw(",\"traceBudgetBytes\":")&&w.U64(full?kTwoSlotFullTraceBytes:kTwoSlotTraceBytes)&&w.Raw(",\"pageBytes\":16384,\"fullBranchMode\":")&&w.Bool(full)&&w.Raw(",\"fullBranchRequiresSeparateCredit\":true}");
}
struct ReadScope {
 ControlIdentityV4 identity;uint64_t reset=0,source=0,physics=0,generation=0;
 bool operator==(const ReadScope& b)const{return identity==b.identity&&reset==b.reset&&source==b.source&&physics==b.physics&&generation==b.generation;}
};
class ReadGate {
 ReadScope scope_;bool active_=false;size_t remaining_=0;
public:
 ControlResult Begin(const ReadScope& expected,const ReadScope& actual,size_t reservedBytes){
  if(!(expected==actual))return ControlResult::STALE_REJECTED;
  if(active_||reservedBytes<kPageBytes||reservedBytes>kDiagnosticBytes)return ControlResult::WOULD_BLOCK;
  scope_=actual;remaining_=reservedBytes;active_=true;return ControlResult::OK;
 }
 ControlResult Page(const ReadScope& actual,size_t requestedBytes){
  if(!active_||!(scope_==actual))return ControlResult::STALE_REJECTED;
  if(!requestedBytes||requestedBytes>kPageBytes||requestedBytes>remaining_)return ControlResult::WOULD_BLOCK;
  remaining_-=requestedBytes;return ControlResult::OK;
 }
 void End(){active_=false;remaining_=0;}
};
}
