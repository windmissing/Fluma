#pragma once
#include "buffer_plan.h"
#include "quota_reader_v3.h"
#include "../cold_protocol.h"
#include "../program_cache/sha256.h"
#include "cold_layout_generated.h"
namespace grs::m3b::cold {
inline constexpr const char* kRevision="T527_V5_BOUNDED_COLD_V1";
inline constexpr const char* kPayload="GRS_M3B_LINEAGE_COLD_V5_1";
inline constexpr size_t kPageBytes=16384,kPageWords=1024,kMaximumTraceBytes=48219008,kMaximumFullTraceBytes=262187776;
inline constexpr size_t kTwoSlotTraceBytes=524375552,kTwoSlotFullTraceBytes=524375552;
using Writer=cold503::Writer;
inline uint64_t Pair(const uint32_t* p,size_t at){return uint64_t(p[at])|(uint64_t(p[at+1])<<32);}
inline void Put(uint32_t* p,size_t at,uint64_t n){p[at]=uint32_t(n);p[at+1]=uint32_t(n>>32);}
inline bool Quota(Writer& w,const QuotaSnapshotV3& q){
 if(InspectQuotaSnapshotV3(q).result!=ControlResult::OK)return false;
 w.Raw("{\"revision\":\"GRS_M3_QUOTA_SNAPSHOT_V3\",\"words\":[");for(size_t i=0;i<q.size();++i){if(i)w.Raw(",");w.Number(q[i]);}return w.Raw("]}");
}
inline bool Capabilities(Writer& w,const Profile& p,size_t alignment,bool full=false){
 Layout layout;if(!Layout::Make(p,alignment,layout,full))return false;ResidentBuffersV5 allocation;if(!ResidentBuffersV5::Make(p,alignment,allocation))return false;
 w.Raw("{\"revision\":\"T527_V5_BOUNDED_COLD_V1\",\"state\":\"PREFIX_DIGEST_POLICY_REQUIRED\",\"supported\":false,\"payloadRevision\":\"GRS_M3B_LINEAGE_COLD_V5_1\",\"pageRevision\":\"T527_SEGMENT_PAGE_V1\",\"route\":\"m3b\",\"wire\":5,\"features\":31,\"channelBitmap\":15,\"pageBytes\":16384,\"pageWords\":1024,\"slotCount\":2,\"builders\":1,\"readers\":1,\"ttlNs\":\"60000000000\",\"residentPayloadBytes\":");w.U64(allocation.total);
 w.Raw(",\"traceBytes\":");w.U64(layout.words*4);w.Raw(",\"traceBudgetBytes\":\"524375552\",\"pages\":");w.Number(layout.Pages());w.Raw(",\"archiveUtf8Limit\":");w.U64((layout.Pages()+1)*kPageBytes);w.Raw(",\"fullBranchMode\":");w.Bool(full);return w.Raw("}");
}
inline void Digest(const uint32_t* words,size_t count,uint32_t* digest){
 p011gpu::Sha256 sha;for(size_t i=0;i<count;++i){const auto v=words[i];const uint8_t b[]={uint8_t(v),uint8_t(v>>8),uint8_t(v>>16),uint8_t(v>>24)};sha.update(b,4);}
 const auto bytes=sha.finish();for(size_t i=0;i<8;++i)digest[i]=uint32_t(bytes[4*i])|(uint32_t(bytes[4*i+1])<<8)|(uint32_t(bytes[4*i+2])<<16)|(uint32_t(bytes[4*i+3])<<24);
}
inline bool Validate(const uint32_t* b,size_t words,const Layout& l){
 if(!b||words<l.words||b[0]!=826488150u||b[1]!=1u||b[2]!=256u||b[3]!=3235775745u||b[4]!=5u||b[5]!=31u||b[6]!=15u||b[11]!=l.count||Pair(b,44)!=l.words*4)return false;
 size_t previous=0;for(size_t i=0;i<l.count;++i){const auto& s=l.segments[i];const auto* d=b+256+i*32;
  if(s.begin!=previous||s.bytes%4||s.stride==0||s.stride>1024||s.bytes%(s.stride*4)||s.begin/4>words||s.bytes/4>words-s.begin/4)return false;previous=s.begin+s.bytes;
  if(d[0]!=i+1||Pair(d,9)!=s.begin||Pair(d,11)!=s.bytes||d[13]!=s.stride||d[14]>s.bytes/(4*s.stride)||d[16]>d[15]||d[14]!=d[16]||d[31]!=3235775745u||d[21]&~7u)return false;
 }
 return previous==l.words*4;
}
inline bool Page(Writer& w,const uint32_t* b,const Layout& l,uint64_t id,const cold503::Scope& scope,uint32_t page){
 if(page>=l.Pages())return false;
 w.Raw("{\"revision\":\"T527_SEGMENT_PAGE_V1\",\"payloadRevision\":\"GRS_M3B_LINEAGE_COLD_V5_1\",\"state\":\"TRACE_READY\",\"traceId\":");w.U64(id);w.Raw(",\"scope\":");cold503::ScopeJson(w,scope);w.Raw(",\"page\":");w.Number(page);w.Raw(",\"pages\":");w.Number(l.Pages());
 if(page==0){w.Raw(",\"segmentCount\":");w.Number(l.count);w.Raw(",\"capacityBytes\":");w.U64(l.words*4);return w.Raw(",\"words\":[]}");}
 size_t first=1;for(size_t segment=0;segment<l.count;++segment){const auto& s=l.segments[segment];const size_t rows=s.bytes/(4*s.stride),per=1024/s.stride,pages=(rows+per-1)/per;
  if(page>=first&&page<first+pages){const size_t row=(page-first)*per,n=std::min(per,rows-row),start=s.begin/4+row*s.stride;
   w.Raw(",\"segment\":");w.Number(segment);w.Raw(",\"segmentName\":");w.String(s.name,std::strlen(s.name));w.Raw(",\"stride\":");w.Number(s.stride);w.Raw(",\"firstRow\":");w.Number(row);w.Raw(",\"rows\":");w.Number(n);w.Raw(",\"wordOffset\":");w.Number(start);w.Raw(",\"words\":[");for(size_t i=0;i<n*s.stride;++i){if(i)w.Raw(",");w.Number(b[start+i]);}return w.Raw("]}");
  }first+=pages;
 }return false;
}
}
