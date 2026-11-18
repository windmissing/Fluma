#pragma once
#include "buffer_plan.h"
#include "capture_schema_generated.h"
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
inline bool Capabilities(Writer& w,const Profile& p,size_t alignment,bool full=false,size_t fixedMetadata=0){
 Layout layout;if(!Layout::Make(p,alignment,layout,full))return false;ResidentBuffersV5 allocation;if(!ResidentBuffersV5::Make(p,alignment,allocation))return false;
 w.Raw("{\"revision\":\"T527_V5_BOUNDED_COLD_V2\",\"state\":\"PREFIX_IDENTITY_CONTRACT_REQUIRED\",\"supported\":false,\"captureScopeMetaRevision\":2,\"payloadRevision\":\"GRS_M3B_LINEAGE_COLD_V5_2\",\"pageRevision\":\"T527_SEGMENT_PAGE_V2\",\"route\":\"m3b\",\"wire\":5,\"features\":31,\"channelBitmap\":15,\"pageBytes\":16384,\"pageWords\":1024,\"slotCount\":2,\"builders\":1,\"readers\":1,\"ttlNs\":\"60000000000\",\"residentPayloadBytes\":");w.U64(allocation.total);w.Raw(",\"fixedCaptureMetadataBytesExcludedFromPayload\":");w.U64(fixedMetadata);
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
// T614: only immutable entry9/10 and same-role final0..3 authorize ranges.
// Return 1=proved skipped, 2=complete, 4=forensic, 0=missing/contradictory.
inline uint32_t BuildScopeMeta(uint32_t* meta,uint32_t root,uint32_t role,const uint32_t* control,const uint32_t* entry,const uint32_t* final){
 std::memset(meta,0,128);
 if(control[0]!=876825680u||control[1]!=5u||control[31]!=3235774468u||control[7]!=role||control[8])return 0;
 if(control[44]&8u)return 1;
 if(entry[11]!=1u||entry[17]!=capture614::kRawRevision||entry[31]!=3235775745u||final[11]!=1u||final[17]!=capture614::kRawRevision||final[31]!=3235775745u)return 0;
 for(size_t i=9;i<=17;++i)if(entry[i]!=final[i])return 0;
 if(!ZeroWords(entry+18,13)||!ZeroWords(final+18,13)||Pair(entry,12)!=Pair(control,4)||entry[14]!=role||Pair(entry,15)!=Pair(control,10))return 0;
 const auto ps=entry[9],as=entry[10];if(final[0]<ps||final[1]<as||final[2]<ps||final[3]<as||final[2]>final[0]||final[3]>final[1])return 0;
 const bool complete=final[4]==0&&final[0]==final[2]&&final[1]==final[3]&&final[7]==0&&control[47]==0&&control[27]==final[2]-ps;
 meta[0]=894848323u;meta[1]=capture614::kMetaRevision;meta[2]=root;meta[3]=role;Put(meta,4,Pair(entry,12));Put(meta,6,Pair(entry,15));meta[8]=ps;meta[9]=final[0];meta[10]=final[2];meta[11]=as;meta[12]=final[1];meta[13]=final[3];meta[14]=1;meta[15]=complete?1:0;meta[16]=final[7]?final[7]:control[47];meta[17]=2;meta[31]=3235775745u;return complete?2u:4u;
}
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
  if(d[0]!=i+1||d[1]!=root||d[2]!=role||d[3]||d[6]!=(i?i:0)||Pair(d,7)!=(i?Pair(b,18):0)||Pair(d,9)!=s.begin||Pair(d,11)!=s.bytes||d[13]!=s.stride||d[15]>rows||d[16]>d[15]||d[14]!=d[16]||d[22]!=(!i||l.fullBranch&&i>=6&&i<=10?2u:1u)||d[31]!=3235775745u||(flags!=1u&&flags!=2u&&flags!=4u))return false;
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
  const auto* cd=b+256+32*(6+root);const auto* control=b+l.segments[6+root].begin/4;
  if(r[7]==1u){if(cd[21]!=1u)return false;continue;}
  if(r[0]==1u){if(cd[21]!=2u||cd[14]||cd[15]||!ZeroWords(control,2048))return false;continue;}
  for(uint32_t slot=0;slot<6;++slot){const auto* row=control+128*slot;if(row[0]!=876825680u||row[1]!=5u||row[31]!=3235774468u||Pair(row,4)!=Pair(r,8))return false;}
  if(!l.fullBranch){if(!ZeroWords(control+768,1280))return false;continue;}
  if(!ZeroWords(control+928,1120))return false;
  for(uint32_t role=1;role<=5;++role){const auto* row=control+128*(role<=3?role-1:role);const auto* meta=control+768+32*(role-1);const auto* p=b+256+32*(21+10*(root-1)+2*(role-1));const auto* a=p+32;
   if(row[7]!=role||row[8])return false;
   if(row[44]&8u){if(!ZeroWords(meta,32)||p[21]!=1u||a[21]!=1u)return false;continue;}
   if(meta[0]!=894848323u||meta[1]!=capture614::kMetaRevision||meta[2]!=root||meta[3]!=role||Pair(meta,4)!=Pair(r,8)||Pair(meta,6)!=Pair(row,10)||meta[14]!=1u||meta[17]!=2u||meta[18]||!ZeroWords(meta+19,12)||meta[31]!=3235775745u||meta[8]>meta[10]||meta[10]>meta[9]||meta[11]>meta[13]||meta[13]>meta[12])return false;
   if(p[14]!=meta[10]-meta[8]||p[15]!=meta[9]-meta[8]||a[14]!=meta[13]-meta[11]||a[15]!=meta[12]-meta[11]||Pair(p,4)!=Pair(meta,6)||Pair(a,4)!=Pair(meta,6))return false;
   if(meta[15]&&(meta[16]||meta[9]!=meta[10]||meta[12]!=meta[13]||p[21]!=2u||a[21]!=2u))return false;
  }

 }
 return true;
}
inline bool Page(Writer& w,const uint32_t* b,const Layout& l,uint64_t id,const cold503::Scope& scope,uint32_t page){
 if(page>=l.Pages())return false;
 w.Raw("{\"revision\":\"T527_SEGMENT_PAGE_V2\",\"payloadRevision\":\"GRS_M3B_LINEAGE_COLD_V5_2\",\"state\":\"TRACE_READY\",\"traceId\":");w.U64(id);w.Raw(",\"requestId\":");w.U64(Pair(b,16));w.Raw(",\"mode\":");w.Number(b[7]);w.Raw(",\"K\":");w.Number(b[8]);w.Raw(",\"Kw\":");w.Number(b[9]);w.Raw(",\"scope\":");cold503::ScopeJson(w,scope);w.Raw(",\"page\":");w.Number(page);w.Raw(",\"pages\":");w.Number(l.Pages());
 if(page==0){w.Raw(",\"canonicalPrefixSha256\":");HexDigest(w,b+279);uint32_t rawPrefix[8];Digest(b,8192,rawPrefix);w.Raw(",\"rawPrefixSha256\":");HexDigest(w,rawPrefix);w.Raw(",\"segmentCount\":");w.Number(l.count);w.Raw(",\"capacityBytes\":");w.U64(l.words*4);return w.Raw(",\"words\":[]}");}
 size_t first=1;for(size_t segment=0;segment<l.count;++segment){const auto& s=l.segments[segment];const size_t rows=s.bytes/(4*s.stride),per=1024/s.stride,pages=(rows+per-1)/per;
  if(page>=first&&page<first+pages){const size_t row=(page-first)*per,n=std::min(per,rows-row),start=s.begin/4+row*s.stride;
   w.Raw(",\"segment\":");w.Number(segment);w.Raw(",\"segmentIndex\":");w.Number(segment);w.Raw(",\"byteOffset\":");w.Number(start*4);w.Raw(",\"rowCount\":");w.Number(n);w.Raw(",\"segmentName\":");w.String(s.name,std::strlen(s.name));w.Raw(",\"stride\":");w.Number(s.stride);w.Raw(",\"firstRow\":");w.Number(row);w.Raw(",\"rows\":");w.Number(n);w.Raw(",\"wordOffset\":");w.Number(start);w.Raw(",\"words\":[");for(size_t i=0;i<n*s.stride;++i){if(i)w.Raw(",");w.Number(b[start+i]);}return w.Raw("]}");
  }first+=pages;
 }return false;
}
}
