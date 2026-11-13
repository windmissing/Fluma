#pragma once
#include "bridge_timeline.h"
#include <algorithm>
#include "program_cache/sha256.h"
#include <iomanip>
namespace grs {namespace diagnostic492 {
// Independent dictionary: never inject these events into the legacy timeline.
struct Scope {uint64_t epoch=0,run=0,context=0,surface=0,session=0,reset=0,permission=0;};
struct Point {Scope scope{};uint64_t ns=0,tid=0,root=0,a=0,b=0,c=0,callStart=0,callEnd=0;uint32_t code=0,gl=0,egl=0;std::array<char,48> stage{};};
inline void ScopeJson(std::ostream& o,const Scope& s){o<<"{\"epoch\":\""<<s.epoch<<"\",\"run\":\""<<s.run<<"\",\"context\":\""<<s.context<<"\",\"surface\":\""<<s.surface<<"\",\"session\":\""<<s.session<<"\",\"resetSerial\":\""<<s.reset<<"\",\"permissionRevision\":\""<<s.permission<<"\"}";}
inline void Json(std::ostream& o,const Point& p){o<<"{\"stage\":\""<<p.stage.data()<<"\",\"clock\":\"STEADY_NS\",\"ns\":\""<<p.ns<<"\",\"tid\":\""<<p.tid<<"\",\"root\":\""<<p.root<<"\",\"a\":\""<<p.a<<"\",\"b\":\""<<p.b<<"\",\"c\":\""<<p.c<<"\",\"callStart\":\""<<p.callStart<<"\",\"callEnd\":\""<<p.callEnd<<"\",\"code\":"<<p.code<<",\"rawGl\":"<<p.gl<<",\"rawEgl\":"<<p.egl<<",\"scope\":";ScopeJson(o,p.scope);o<<'}';}
struct Cause {bool set=false;Point point{};std::array<char,64> reason{};uint64_t latePulses=0;};
struct Root {Scope scope{};uint64_t nonce=0,started=0,deadline=0,kind=0,plan=0;Identity identity{};U64 input{};};
struct Group {bool used=false;Scope scope{};Cause cause{};Root root{},quantum{},faultRoot{};std::array<Point,768> first{},firstQuantum{};std::array<Point,64> tail{};uint32_t count=0,quantumCount=0,tailCount=0;uint64_t dropped=0,quantumDropped=0,tailDropped=0;};
struct Summary {uint64_t count=0,enters=0,exits=0;Point first{},last{},firstError{};bool error=false;};
struct Action {Point point{};std::array<char,1025> raw{};};
struct Packet {Point point{};std::array<uint32_t,32> words{};std::array<char,64> reply{};uint32_t hash=0;};
struct Capture {
 std::array<Group,4> groups{};std::array<Summary,128> stages{};std::array<Action,96> actions{};
 std::array<Packet,64> packets{};uint32_t packetCount=0;uint64_t packetDropped=0;
 std::array<Point,32> resets{};uint32_t resetCount=0;uint64_t resetDropped=0;
 void ResetRecord(const Point& point){if(resetCount<resets.size())resets[resetCount++]=point;else ++resetDropped;}
 uint32_t groupCount=0,stageCount=0,actionCount=0;uint64_t scopeDropped=0,unknownStageCount=0,actionDropped=0;
 Group* Find(const Scope& s){for(uint32_t i=0;i<groupCount;++i){auto& g=groups[i];if(g.scope.epoch==s.epoch&&g.scope.run==s.run&&g.scope.session==s.session)return &g;}return nullptr;}
 Group* Get(const Scope& s){if(auto* g=Find(s))return g;if(groupCount==groups.size()){++scopeDropped;return nullptr;}auto& g=groups[groupCount++];g.used=true;g.scope=s;return &g;}
 void Add(const Point& p){
  Summary* summary=nullptr;
  for(uint32_t i=0;i<stageCount;++i)if(stages[i].first.stage==p.stage){summary=&stages[i];break;}
  if(!summary&&stageCount<stages.size()){summary=&stages[stageCount++];summary->first=p;}
  if(summary){++summary->count;summary->last=p;const auto* name=p.stage.data();if(std::strstr(name,"ENTER"))++summary->enters;if(std::strstr(name,"EXIT")||std::strstr(name,"RETURN"))++summary->exits;if(p.code&&!summary->error){summary->error=true;summary->firstError=p;}}
  else ++unknownStageCount;
  auto* g=Find(p.scope);if(!g)return;
  if(g->cause.set&&p.root==g->cause.point.root&&p.root){if(g->tailCount<g->tail.size())g->tail[g->tailCount++]=p;else ++g->tailDropped;}
  else if(g->root.nonce&&(p.root==g->root.nonce||p.root==g->quantum.nonce)){
   if(g->cause.set)return;
   else if(p.root==g->root.nonce){if(g->count<g->first.size())g->first[g->count++]=p;else ++g->dropped;}
   else {if(g->quantumCount<g->firstQuantum.size())g->firstQuantum[g->quantumCount++]=p;else ++g->quantumDropped;}
  }
 }
 void Fault(const Point& p,const char* reason,const Root& root){auto* g=Get(p.scope);if(!g||g->cause.set)return;g->cause.set=true;g->cause.point=p;g->faultRoot=root;std::strncpy(g->cause.reason.data(),reason,g->cause.reason.size()-1);}
 void Intent(const Root& r){auto* g=Get(r.scope);if(!g)return;if(!g->root.nonce)g->root=r;if(r.kind==2&&!g->quantum.nonce)g->quantum=r;}
 void ActionRecord(const Point& p,const std::string& raw){if(actionCount==actions.size()||raw.size()>1024){++actionDropped;return;}auto& a=actions[actionCount++];a.point=p;std::memcpy(a.raw.data(),raw.data(),raw.size());}
 void PacketRecord(const Point& p,const std::array<uint32_t,32>& words,const char* result){
  if(packetCount==packets.size()){++packetDropped;return;}auto& packet=packets[packetCount++];packet.point=p;packet.words=words;
  std::strncpy(packet.reply.data(),result,packet.reply.size()-1);uint32_t hash=2166136261u;for(auto word:words)for(unsigned i=0;i<4;++i){hash^=uint8_t(word>>(8*i));hash*=16777619u;}packet.hash=hash;
 }
 void Json(std::ostream& o)const{
  o<<"{\"revision\":\"T492_EXECUTION_DIAGNOSTICS_V1\",\"clock\":\"STEADY_NS\",\"bytes\":"<<sizeof(*this)<<",\"scopeDropped\":"<<scopeDropped<<",\"unknownStageCount\":"<<unknownStageCount<<",\"actionDropped\":"<<actionDropped<<",\"stages\":[";
  for(uint32_t i=0;i<stageCount;++i){if(i)o<<',';const auto& s=stages[i];o<<"{\"count\":\""<<s.count<<"\",\"enters\":\""<<s.enters<<"\",\"exits\":\""<<s.exits<<"\",\"first\":";diagnostic492::Json(o,s.first);o<<",\"last\":";diagnostic492::Json(o,s.last);o<<",\"firstError\":";if(s.error)diagnostic492::Json(o,s.firstError);else o<<"null";o<<'}';}
  o<<"],\"groups\":[";
  for(uint32_t i=0;i<groupCount;++i){if(i)o<<',';const auto& g=groups[i];o<<"{\"scope\":";ScopeJson(o,g.scope);o<<",\"firstCause\":";
   if(g.cause.set){o<<"{\"reason\":\""<<g.cause.reason.data()<<"\",\"point\":";diagnostic492::Json(o,g.cause.point);o<<",\"latePulses\":\""<<g.cause.latePulses<<"\"}";}else o<<"null";
   o<<",\"root\":{\"nonce\":\""<<g.root.nonce<<"\",\"started\":\""<<g.root.started<<"\",\"deadline\":\""<<g.root.deadline<<"\",\"kind\":"<<g.root.kind<<",\"plan\":\""<<g.root.plan<<"\",\"scope\":";ScopeJson(o,g.root.scope);
   o<<",\"input\":["<<g.root.input.lo<<','<<g.root.input.hi<<"],\"identityWords\":[";
   const auto* bytes=reinterpret_cast<const unsigned char*>(&g.root.identity);for(size_t j=0;j<sizeof(Identity);++j){if(j)o<<',';o<<unsigned(bytes[j]);}o<<"]},\"dropped\":"<<g.dropped<<",\"tailDropped\":"<<g.tailDropped<<",\"events\":[";
   for(uint32_t j=0;j<g.count;++j){if(j)o<<',';diagnostic492::Json(o,g.first[j]);}o<<"],\"firstQuantum\":{\"nonce\":\""<<g.quantum.nonce<<"\",\"started\":\""<<g.quantum.started<<"\",\"deadline\":\""<<g.quantum.deadline<<"\",\"plan\":\""<<g.quantum.plan<<"\",\"sameAsFirstRoot\":"<<(g.quantum.nonce&&g.quantum.nonce==g.root.nonce?"true":"false")<<",\"scope\":";ScopeJson(o,g.quantum.scope);o<<",\"dropped\":"<<g.quantumDropped<<",\"events\":[";
   for(uint32_t j=0;j<g.quantumCount;++j){if(j)o<<',';diagnostic492::Json(o,g.firstQuantum[j]);}
   o<<"],\"input\":["<<g.quantum.input.lo<<','<<g.quantum.input.hi<<"],\"identityBytes\":[";const auto* qbytes=reinterpret_cast<const unsigned char*>(&g.quantum.identity);for(size_t j=0;j<sizeof(Identity);++j){if(j)o<<',';o<<unsigned(qbytes[j]);}
   o<<"]},\"faultRoot\":{\"nonce\":\""<<g.faultRoot.nonce<<"\",\"started\":\""<<g.faultRoot.started<<"\",\"deadline\":\""<<g.faultRoot.deadline<<"\",\"kind\":"<<g.faultRoot.kind<<",\"plan\":\""<<g.faultRoot.plan<<"\",\"scope\":";ScopeJson(o,g.faultRoot.scope);o<<",\"input\":["<<g.faultRoot.input.lo<<','<<g.faultRoot.input.hi<<"],\"identityBytes\":[";const auto* fbytes=reinterpret_cast<const unsigned char*>(&g.faultRoot.identity);for(size_t j=0;j<sizeof(Identity);++j){if(j)o<<',';o<<unsigned(fbytes[j]);}
   o<<"]},\"tail\":[";for(uint32_t j=0;j<g.tailCount;++j){if(j)o<<',';diagnostic492::Json(o,g.tail[j]);}o<<"]}";
  }
  o<<"],\"actions\":[";for(uint32_t i=0;i<actionCount;++i){if(i)o<<',';o<<"{\"native\":";diagnostic492::Json(o,actions[i].point);o<<",\"raw\":";
   // Preserve untrusted UI bytes as hex, not interpolated JSON or strings.
   o<<'"';constexpr char h[]="0123456789abcdef";for(unsigned char c:actions[i].raw){if(!c)break;o<<h[c>>4]<<h[c&15];}o<<"\"}";
  }o<<"],\"packetDropped\":"<<packetDropped<<",\"packets\":[";
  for(uint32_t i=0;i<packetCount;++i){if(i)o<<',';const auto& p=packets[i];o<<"{\"native\":";diagnostic492::Json(o,p.point);o<<",\"request\":"<<p.words[6]<<",\"hashRevision\":\"FNV1A32_LE_128B\",\"hash\":\""<<std::hex<<std::setfill('0')<<std::setw(8)<<p.hash<<std::dec<<"\",\"reply\":\""<<p.reply.data()<<"\",\"commandSHA256\":\"";
   p011gpu::Sha256 sha;for(auto word:p.words){const uint8_t bytes[]={uint8_t(word),uint8_t(word>>8),uint8_t(word>>16),uint8_t(word>>24)};sha.update(bytes,4);}for(auto b:sha.finish())o<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(b);o<<std::dec<<"\"}";
  }o<<"],\"resetDropped\":"<<resetDropped<<",\"resets\":[";for(uint32_t i=0;i<resetCount;++i){if(i)o<<',';diagnostic492::Json(o,resets[i]);}o<<"]}";
 }
};
// GL owner callback. No GL queries or changes of control state in a probe.
struct Observer {void* user=nullptr;void(*write)(void*,const char*,uint64_t,uint64_t,uint32_t,uint32_t,uint32_t)=nullptr;
 void Emit(const char* stage,uint64_t a=0,uint64_t b=0,uint32_t code=0,uint32_t gl=0,uint32_t egl=0)const{if(write)write(user,stage,a,b,code,gl,egl);}};
inline thread_local Observer ownerObserver{};
inline thread_local uint32_t lastSwapError=0;
}}
