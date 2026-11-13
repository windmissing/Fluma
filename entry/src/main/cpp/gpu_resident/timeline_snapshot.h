#pragma once
#include "bridge_timeline.h"
#include "program_cache/sha256.h"
#include <limits>
#include <memory>
namespace grs {
// T492: one fixed payload. All mutation/copy methods require App::mutex.
// Page is a bounded copy so JSON and disk I/O never hold that mutex.
struct SnapshotScope {
 uint64_t epoch=0,run=0,context=0,surface=0,pid=0;
 std::array<char,129> package{},route{},source{};
};
inline void SnapshotScopeJson(std::ostream& o,const SnapshotScope& s){
 o<<"{\"epoch\":\""<<s.epoch<<"\",\"run\":\""<<s.run<<"\",\"context\":\""<<s.context
  <<"\",\"surface\":\""<<s.surface<<"\",\"pid\":\""<<s.pid<<"\",\"packageSource\":\""<<s.package.data()
  <<"\",\"route\":\""<<s.route.data()<<"\",\"source\":\""<<s.source.data()<<"\"}";
}
struct TimelineSnapshot {
 static constexpr uint64_t ttlNs=60000000000ULL;
 struct Payload {std::array<BridgeTimeRow,4096> rows{};std::array<BridgeTimeline::Tokens,32> tokens{};};
 static_assert(sizeof(Payload)==753664,"T489 fixed snapshot budget");
 Payload payload;
 uint64_t sequence=0,id=0,capturedNs=0,copyNs=0,serial=0,dropped=0;
 uint32_t count=0,tokenCount=0;SnapshotScope scope{};
 std::array<char,40> dictionary{};
 bool active=false;
 struct Page {
  uint64_t id=0,capturedNs=0,copyNs=0,serial=0,dropped=0;
  uint32_t count=0,cursor=0,size=0;SnapshotScope scope{};std::array<char,40> dictionary{};
  std::array<BridgeTimeRow,4> rows{};std::array<BridgeTimeline::Tokens,4> tokens{};
  std::string Json()const{
   std::ostringstream o;
   o<<"{\"snapshotRevision\":\"T492_IMMUTABLE_TIMELINE_V1\",\"snapshotId\":\""<<id
    <<"\",\"capturedNs\":\""<<capturedNs<<"\",\"copyNs\":\""<<copyNs<<"\",\"scope\":";SnapshotScopeJson(o,scope);
   o<<",\"revision\":\"T295_STEADY_TRACE_V1\",\"clock\":\"STEADY_NS\",\"capacity\":4096,\"rowBytes\":"<<sizeof(BridgeTimeRow)
    <<",\"payloadBytes\":753664,\"stageRevision\":\""<<dictionary.data()<<"\",\"count\":"<<count<<",\"serial\":\""<<serial
    <<"\",\"dropped\":\""<<dropped<<"\",\"incomplete\":"<<(dropped?"true":"false")<<",\"cursor\":"<<cursor<<",\"rows\":[";
   for(uint32_t i=0;i<size;++i){const auto& r=rows[i];if(i)o<<',';
    o<<"{\"serial\":\""<<r.serial<<"\",\"ns\":\""<<r.ns<<"\",\"tid\":\""<<r.tid<<"\",\"epoch\":\""<<r.epoch<<"\",\"run\":\""<<r.run
     <<"\",\"surface\":\""<<r.surface<<"\",\"input\":\""<<r.input<<"\",\"now\":\""<<r.now<<"\",\"last\":\""<<r.last<<"\",\"delta\":\""<<r.delta
     <<"\",\"root\":\""<<r.root<<"\",\"stage\":"<<r.stage<<",\"code\":"<<r.code<<",\"carry\":"<<r.carry<<",\"quanta\":"<<r.quanta
     <<",\"appQueue\":"<<r.appQueue<<",\"runQueue\":"<<r.runQueue<<",\"flags\":"<<r.flags;
    if(r.stage>=43){o<<",\"probeStart\":\""<<r.probeStart<<"\",\"probeEnd\":\""<<r.probeEnd<<"\",\"probeA\":\""<<r.probeA
     <<"\",\"probeB\":\""<<r.probeB<<"\",\"probeC\":\""<<r.probeC<<"\",\"probeWriteNs\":\""<<r.probeWriteNs<<"\"";
     if(r.stage==50){if(r.receipt)o<<",\"requestedToken\":\""<<tokens[i].requested.data()<<"\",\"renderedToken\":\""<<tokens[i].rendered.data()<<'"';else o<<",\"tokenEvidence\":\"MISSING_BOUNDED_RECEIPT\"";}}
    o<<'}';
   }
   o<<"],\"next\":"<<cursor+size<<",\"completePage\":true}";return o.str();
  }
 };
 void Expire(uint64_t now){if(active&&(now<capturedNs||now-capturedNs>=ttlNs))active=false;}
 const char* Begin(const BridgeTimeline& t,const SnapshotScope& s,uint64_t now){
  Expire(now);if(active)return "SNAPSHOT_BUSY";
  if(sequence==UINT64_MAX)return "SNAPSHOT_ID_EXHAUSTED";
  const auto begin=BridgeTimeline::Now();
  std::copy_n(t.rows.begin(),t.count,payload.rows.begin());
  std::copy_n(t.tokens.begin(),t.tokenCount,payload.tokens.begin());
  count=t.count;tokenCount=t.tokenCount;serial=t.serial;dropped=t.dropped;scope=s;
  const char* revision=t.isolationDictionary?"T477_STAGE67_V1":t.coldDictionary?"T389_STAGE57_V1":t.tailDictionary?"T384_STAGE50_V1":"T324_STAGE42_V1";
  dictionary.fill(0);std::strncpy(dictionary.data(),revision,dictionary.size()-1);
  capturedNs=now;copyNs=BridgeTimeline::Now()-begin;id=++sequence;active=true;return "OK";
 }
 const char* Read(uint64_t requested,uint32_t cursor,uint64_t now,Page& p){
  Expire(now);if(!active||requested!=id)return "STALE_SNAPSHOT";
  if(cursor>count)return "INVALID_CURSOR";
  p.id=id;p.capturedNs=capturedNs;p.copyNs=copyNs;p.serial=serial;p.dropped=dropped;p.scope=scope;p.dictionary=dictionary;p.count=count;p.cursor=cursor;
  p.size=std::min(4u,count-cursor);
  for(uint32_t i=0;i<p.size;++i){p.rows[i]=payload.rows[cursor+i];const auto receipt=p.rows[i].receipt;
   if(receipt&&receipt<=tokenCount)p.tokens[i]=payload.tokens[receipt-1];else p.rows[i].receipt=0;}
  return "OK";
 }
 const char* End(uint64_t requested,uint64_t now){Expire(now);if(!active||id!=requested)return "STALE_SNAPSHOT";active=false;return "SNAPSHOT_RELEASED";}
};
}
