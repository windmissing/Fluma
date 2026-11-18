#pragma once
#include "buffer_plan.h"
#include "quota_reader_v3.h"
#include "../cold_protocol.h"
#include "../program_cache/sha256.h"
#include "cold_layout_generated.h"
namespace grs::m3b::cold {
inline constexpr const char* kRevision="T527_V5_BOUNDED_COLD_V2";
inline constexpr const char* kPayload="GRS_M3B_LINEAGE_COLD_V5_2";
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
 w.Raw("{\"revision\":\"T527_V5_BOUNDED_COLD_V2\",\"state\":\"META_INCOMPLETE\",\"supported\":false,\"payloadRevision\":\"GRS_M3B_LINEAGE_COLD_V5_2\",\"pageRevision\":\"T527_SEGMENT_PAGE_V2\",\"route\":\"m3b\",\"wire\":5,\"features\":31,\"channelBitmap\":15,\"pageBytes\":16384,\"pageWords\":1024,\"slotCount\":2,\"builders\":1,\"readers\":1,\"ttlNs\":\"60000000000\",\"residentPayloadBytes\":");w.U64(allocation.total);
 w.Raw(",\"traceBytes\":");w.U64(layout.words*4);w.Raw(",\"traceBudgetBytes\":\"524375552\",\"pages\":");w.Number(layout.Pages());w.Raw(",\"archiveUtf8Limit\":");w.U64((layout.Pages()+1)*kPageBytes);w.Raw(",\"fullBranchMode\":");w.Bool(full);return w.Raw("}");
}
inline void Digest(const uint32_t* words,size_t count,uint32_t* digest){
 p011gpu::Sha256 sha;for(size_t i=0;i<count;++i){const auto v=words[i];const uint8_t b[]={uint8_t(v),uint8_t(v>>8),uint8_t(v>>16),uint8_t(v>>24)};sha.update(b,4);}
 const auto bytes=sha.finish();for(size_t i=0;i<8;++i)digest[i]=uint32_t(bytes[4*i])|(uint32_t(bytes[4*i+1])<<8)|(uint32_t(bytes[4*i+2])<<16)|(uint32_t(bytes[4*i+3])<<24);
}
inline void PrefixDigest(const uint32_t* b,uint32_t* digest){
 p011gpu::Sha256 sha;for(size_t i=0;i<8192;++i){const uint32_t v=(i>=279&&i<287)?0:b[i];const uint8_t bytes[]={uint8_t(v),uint8_t(v>>8),uint8_t(v>>16),uint8_t(v>>24)};sha.update(bytes,4);}const auto bytes=sha.finish();for(size_t i=0;i<8;++i)digest[i]=uint32_t(bytes[4*i])|(uint32_t(bytes[4*i+1])<<8)|(uint32_t(bytes[4*i+2])<<16)|(uint32_t(bytes[4*i+3])<<24);
}
inline void HexDigest(Writer& w,const uint32_t* words){uint8_t raw[32];for(size_t i=0;i<8;++i)for(unsigned j=0;j<4;++j)raw[4*i+j]=uint8_t(words[i]>>(8*j));w.Hex(raw,32);}
// Only cold-owned immutable spans enter this validator. No live GPU access.
inline bool ZeroWords(const uint32_t* p,size_t count){for(size_t i=0;i<count;++i)if(p[i])return false;return true;}
inline bool Validate(const uint32_t* b,size_t words,const Layout& l){
 if(!b||words<l.words||b[0]!=826488150u||b[1]!=2u||b[2]!=256u||b[3]!=3235775746u||b[4]!=5u||b[5]!=31u||b[6]!=15u||b[7]!=(l.fullBranch?2u:1u)||b[10]!=(l.fullBranch?5u:1u)||b[11]!=l.count||b[12]>1u||b[13]!=0u||b[14]!=l.Pages()-1||b[15]!=1u||!Pair(b,18)||Pair(b,42)!=l.words*4||Pair(b,44)!=l.words*4||Pair(b,46)!=10224000u)return false;
 if(!ZeroWords(b+100,156)||!ZeroWords(b+256+l.count*32,(64-l.count)*32))return false;
 uint32_t hash[8];PrefixDigest(b,hash);if(std::memcmp(hash,b+279,32))return false;
 size_t previous=0;
 for(size_t i=0;i<l.count;++i){const auto& s=l.segments[i];const auto* d=b+256+i*32;
  if(s.begin!=previous||s.bytes%4||!s.stride||s.stride>1024||s.bytes%(s.stride*4)||s.begin/4>words||s.bytes/4>words-s.begin/4)return false;previous=s.begin+s.bytes;
  const uint32_t root=i==0?0u:!l.fullBranch?0u:i<=5?UINT32_MAX:i<=10?uint32_t(i-6):i<=20?uint32_t((i-11)/2):uint32_t((i-21)/10+1);
  const uint32_t role=l.fullBranch&&i>=21?uint32_t((i-21)%10/2+1):0u;
  const uint32_t flags=d[21];const auto rows=s.bytes/(4*s.stride);
  if(d[0]!=i+1||d[1]!=root||d[2]!=role||d[3]||d[6]!=(i?i:0)||Pair(d,7)!=(i?Pair(b,18):0)||Pair(d,9)!=s.begin||Pair(d,11)!=s.bytes||d[13]!=s.stride||d[15]>rows||d[16]>d[15]||d[14]!=d[16]||d[22]!=(i?1u:2u)||d[31]!=3235775745u||(flags!=1u&&flags!=2u&&flags!=4u))return false;
  if(!role&&Pair(d,4))return false;
  if(!i){if(flags!=2||d[14]!=8192||d[15]!=8192||d[17]||d[18]||d[19]||d[20])return false;continue;}
  const auto* body=b+s.begin/4;
  if(flags==1u){if(d[14]||d[15]||Pair(d,4)||d[17]||d[18]||d[19]||d[20]||!ZeroWords(body,s.bytes/4))return false;}
  if(flags==4u&&!(b[12]&1u))return false;
  if(flags==2u&&d[14]!=d[15])return false;
  Digest(body,s.bytes/4,hash);if(std::memcmp(hash,d+23,32))return false;
 }
 if(previous!=l.words*4)return false;
 for(uint32_t root=0;root<b[10];++root){const auto* r=b+6048+root*128;
  if((r[0]!=1&&r[0]!=2)||l.fullBranch&&r[0]!=(root?2u:1u)||(r[7]!=1u&&r[7]!=2u&&r[7]!=4u)||r[30]!=2304+256*root||r[31]!=3584+64*root||r[32]!=3904+256*root||r[33]!=5184+64*root||r[34]!=(l.fullBranch?11+2*root:UINT32_MAX)||r[35]!=(l.fullBranch?12+2*root:UINT32_MAX)||r[36]!=3u||r[37]!=6+root||r[38]!=(l.fullBranch&&root?21+10*(root-1):UINT32_MAX)||r[39]!=(l.fullBranch&&root?5u:0u)||r[40]!=1u||r[41]!=2u||!ZeroWords(r+66,62))return false;
  if(r[7]==2u&&!(b[12]&1u))return false;
  if(r[7]==1u){if(!ZeroWords(r+1,6)||!ZeroWords(r+8,22)||!ZeroWords(r+42,24))return false;}
  else if(!Pair(r,8)||!Pair(r,20)||Pair(r,24)<Pair(r,22))return false;
 }
 return true;
}
inline bool Page(Writer& w,const uint32_t* b,const Layout& l,uint64_t id,const cold503::Scope& scope,uint32_t page){
 if(page>=l.Pages())return false;
 w.Raw("{\"revision\":\"T527_SEGMENT_PAGE_V2\",\"payloadRevision\":\"GRS_M3B_LINEAGE_COLD_V5_2\",\"state\":\"TRACE_READY\",\"traceId\":");w.U64(id);w.Raw(",\"scope\":");cold503::ScopeJson(w,scope);w.Raw(",\"page\":");w.Number(page);w.Raw(",\"pages\":");w.Number(l.Pages());
 if(page==0){w.Raw(",\"segmentCount\":");w.Number(l.count);w.Raw(",\"capacityBytes\":");w.U64(l.words*4);return w.Raw(",\"words\":[]}");}
 size_t first=1;for(size_t segment=0;segment<l.count;++segment){const auto& s=l.segments[segment];const size_t rows=s.bytes/(4*s.stride),per=1024/s.stride,pages=(rows+per-1)/per;
  if(page>=first&&page<first+pages){const size_t row=(page-first)*per,n=std::min(per,rows-row),start=s.begin/4+row*s.stride;
   w.Raw(",\"segment\":");w.Number(segment);w.Raw(",\"segmentName\":");w.String(s.name,std::strlen(s.name));w.Raw(",\"stride\":");w.Number(s.stride);w.Raw(",\"firstRow\":");w.Number(row);w.Raw(",\"rows\":");w.Number(n);w.Raw(",\"wordOffset\":");w.Number(start);w.Raw(",\"words\":[");for(size_t i=0;i<n*s.stride;++i){if(i)w.Raw(",");w.Number(b[start+i]);}return w.Raw("]}");
  }first+=pages;
 }return false;
}
}
