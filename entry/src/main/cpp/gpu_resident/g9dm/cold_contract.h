#pragma once
#include "transport.h"
#include "../cold_protocol.h"
namespace grs::g9dm::cold {
using Writer=cold503::Writer;
inline constexpr size_t kPageBytes=16384,kPageWords=1024,kMaximumTraceBytes=45953024,kMaximumFullTraceBytes=179671040,kTwoSlotTraceBytes=359342080,kTwoSlotFullTraceBytes=359342080;
struct Segment {const char* name;size_t begin,words;uint32_t stride;};
struct Layout {
 std::array<Segment,10> segments{};size_t count=0,words=0;bool fullBranch=false;
 static bool Make(const Profile& p,size_t,Layout& out,bool full=false){
  Layout l;l.fullBranch=full;size_t at=8192;
  l.segments[0]={"BASE",at,p.stateBytes/4,1};at+=p.stateBytes/4;l.segments[1]={"FINAL",at,p.stateBytes/4,1};at+=p.stateBytes/4;
  l.segments[2]={"SELECTED_EVENT_PATH",at,p.arenaBytes/4,1};at+=p.arenaBytes/4;l.segments[3]={"AUTHORITATIVE_EVENT_PATH",at,p.arenaBytes/4,1};at+=p.arenaBytes/4;l.count=4;
  if(full)for(size_t i=0;i<6;++i){l.segments[4+i]={"DISCOVERY_EVENT_PATH",at,p.arenaBytes/4,1};at+=p.arenaBytes/4;}if(full)l.count=10;
  l.words=at;if(at*4!=(full?p.fullColdBytes:p.rootColdBytes)||at*4>(full?kMaximumFullTraceBytes:kMaximumTraceBytes))return false;out=l;return true;
 }
 size_t Pages()const{size_t pages=1+8;for(size_t i=0;i<count;++i){const auto& s=segments[i];pages+=(s.words+1023)/1024;}return pages;}
};
inline void SegmentDigest(const uint32_t* block,size_t words,uint32_t* out){auto digest=HashWords(block,words);std::copy(digest.begin(),digest.end(),out);}
inline bool Validate(const uint32_t* b,size_t capacity,const Layout& l){
 if(!b||capacity<l.words||b[0]!=0x31433947||b[1]!=1||b[2]!=8192||b[3]!=kSeal||b[4]!=kProfileCode||b[5]!=(l.fullBranch?1u:0u)||b[6]!=l.count||Pair(b,8)!=l.words*4)return false;
 if(b[256]!=kCompletionMagic||b[257]!=1||b[258]!=256||b[259]!=kSeal||b[260]!=kProfileCode||b[768]!=860963661||b[769]!=3||b[770]!=256||b[771]!=0xc0de0003)return false;
 for(size_t i=0;i<l.count;++i){const auto& s=l.segments[i];const auto* d=b+2048+i*32;if(d[0]!=i+1||Pair(d,8)!=s.begin||Pair(d,10)!=s.words||d[12]!=1||d[31]!=kSeal||s.begin>capacity||s.words>capacity-s.begin)return false;}
 return true;
}
inline bool Page(Writer& w,const uint32_t* b,const Layout& l,uint64_t trace,const cold503::Scope& scope,uint32_t page){
 if(page>=l.Pages())return false;w.Raw("{\"revision\":\"G9_SPF_SEGMENT_PAGE_V1\",\"payloadRevision\":\"G9_SPF_COLD_V1\",\"state\":\"TRACE_READY\",\"traceId\":");w.U64(trace);w.Raw(",\"scope\":");cold503::ScopeJson(w,scope);w.Raw(",\"page\":");w.Number(page);w.Raw(",\"pages\":");w.Number(l.Pages());
 if(page==0){w.Raw(",\"segmentCount\":");w.Number(l.count);w.Raw(",\"capacityBytes\":");w.U64(l.words*4);w.Raw(",\"K\":");w.Number(b[7]);w.Raw(",\"fullTrialTrace\":");w.Bool(l.fullBranch);return w.Raw(",\"words\":[]}");}
 size_t begin=0,count=0,segment=0,first=1;if(page<=8){begin=(page-1)*1024;count=1024;segment=0;}
 else{first=9;for(size_t i=0;i<l.count;++i){const auto& s=l.segments[i];size_t pages=(s.words+1023)/1024;if(page>=first&&page<first+pages){begin=s.begin+(page-first)*1024;count=std::min<size_t>(1024,s.begin+s.words-begin);segment=i+1;break;}first+=pages;}}
 if(!count)return false;w.Raw(",\"segment\":");w.Number(segment);w.Raw(",\"wordOffset\":");w.Number(begin);w.Raw(",\"stride\":1,\"words\":[");for(size_t i=0;i<count;++i){if(i)w.Raw(",");w.Number(b[begin+i]);}return w.Raw("]}");
}
}
