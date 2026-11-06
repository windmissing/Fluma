#pragma once
#include "control.h"
#include "submit_observer.h"
#include <cstring>
#include <chrono>
#include <thread>
#include <sstream>
#if !defined(_WIN32)
#include <unistd.h>
#endif
namespace grs {
enum class BridgeStage:uint32_t {Invalidate=1,InitBegin,Ready,ReadyRejected,WaitEnter,Wake,FirstTick,AdvanceBegin,AdvanceEnd,AnchorSet,PumpBegin,PumpEnd,DrawBegin,DrawEnd,SwapBegin,SwapEnd,PauseBegin,PauseEnd,Offer,Consume,Stale,RootSubmit,RootPoll,SubmitEnter,SubmitExit,ShaderEnter,ShaderExit,ProgramEnter,ProgramExit,ResourceEnter,ResourceExit,PrepareEnter,PrepareExit,DispatchEnter,DispatchExit,FenceEnter,FenceExit,PollEnter,PollExit};
inline uint64_t BridgeTid(){
#if defined(_WIN32)
 return std::hash<std::thread::id>{}(std::this_thread::get_id()); // host double only
#else
 return uint64_t(gettid()); // original OS worker TID for scheduling correlation
#endif
}
struct BridgeTimeRow {uint64_t serial=0,ns=0,tid=0,epoch=0,run=0,surface=0,input=0,now=0,last=0,delta=0,root=0;uint64_t probeStart=0,probeEnd=0,probeA=0,probeB=0,probeC=0,probeWriteNs=0;uint32_t receipt=0;uint32_t stage=0,code=0,carry=0,quanta=0,appQueue=0,runQueue=0,flags=0;};
struct BridgeTimeline {
 // T335 §4: capacity 1024 -> 4096. Amendment 1's in-Pump multi-batch loop
 // raised per-root event density 50 -> 376 rows (7.5x); at 1024 a multi-run
 // session overflows and every export goes INCOMPLETE (fail-closed fires
 // correctly but the diagnostic data after overflow is lost). 4096 covers
 // ~10.8 roots (+330KB diagnostic-build memory, BridgeTimeRow ~108B x 4096).
 // Page export (4 rows/page) and fail-closed semantics unchanged; NO
 // auto-export ring clearing (cold-export semantics stay untouched).
 static constexpr uint32_t capacity=4096,pageRows=4;std::array<BridgeTimeRow,capacity> rows{};uint64_t serial=0,dropped=0;uint32_t count=0;
 explicit BridgeTimeline(bool hasTailDictionary=false,bool hasColdDictionary=false)
  :tailDictionary(hasTailDictionary),coldDictionary(hasColdDictionary){}
 const bool tailDictionary;
 const bool coldDictionary;
 struct Tokens{std::array<char,1024> requested{},rendered{};};
 std::array<Tokens,32> tokens{};uint32_t tokenCount=0;
 uint32_t SaveTokens(const std::string& requested,const std::string& rendered){
  if(count==capacity||tokenCount==tokens.size()||requested.size()>=1024||rendered.size()>=1024)return 0;
  auto& out=tokens[tokenCount++];
  std::memcpy(out.requested.data(),requested.c_str(),requested.size()+1);
  std::memcpy(out.rendered.data(),rendered.c_str(),rendered.size()+1);
  return tokenCount;
 }
 static uint64_t Now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
 void Add(BridgeTimeRow row){row.serial=++serial;if(count==capacity){++dropped;return;}rows[count++]=row;}
 // Called under App's same lock; explicit bounded cold page, no hot JSON.
 std::string Export(uint32_t cursor)const{std::ostringstream o;o<<"{\"revision\":\"T295_STEADY_TRACE_V1\",\"clock\":\"STEADY_NS\",\"capacity\":"<<capacity<<",\"rowBytes\":"<<sizeof(BridgeTimeRow)<<",\"totalBufferBytes\":"<<sizeof(rows)<<",\"jsonExportBudgetBytes\":1048576,\"count\":"<<count<<",\"stageRevision\":\""<<(coldDictionary?"T389_STAGE57_V1":tailDictionary?"T384_STAGE50_V1":"T324_STAGE42_V1")<<"\",\"serial\":\""<<serial<<"\",\"dropped\":\""<<dropped<<"\""<<",\"incomplete\":"<<(dropped?"true":"false")<<",\"cursor\":"<<cursor<<",\"rows\":[";uint32_t end=std::min(count,cursor+pageRows);for(uint32_t i=cursor;i<end;i++){const auto& r=rows[i];if(i!=cursor)o<<',';o<<"{\"serial\":\""<<r.serial<<"\",\"ns\":\""<<r.ns<<"\",\"tid\":\""<<r.tid<<"\",\"epoch\":\""<<r.epoch<<"\",\"run\":\""<<r.run<<"\",\"surface\":\""<<r.surface<<"\",\"input\":\""<<r.input<<"\",\"now\":\""<<r.now<<"\",\"last\":\""<<r.last<<"\",\"delta\":\""<<r.delta<<"\",\"root\":\""<<r.root<<"\",\"stage\":"<<r.stage<<",\"code\":"<<r.code<<",\"carry\":"<<r.carry<<",\"quanta\":"<<r.quanta<<",\"appQueue\":"<<r.appQueue<<",\"runQueue\":"<<r.runQueue<<",\"flags\":"<<r.flags;
 if(r.stage>=uint32_t(TailStage::BatchCall)){
  o<<",\"probeStart\":\""<<r.probeStart<<"\",\"probeEnd\":\""<<r.probeEnd<<"\",\"probeA\":\""<<r.probeA<<"\",\"probeB\":\""<<r.probeB<<"\",\"probeC\":\""<<r.probeC<<"\",\"probeWriteNs\":\""<<r.probeWriteNs<<'"';
  if(r.stage==uint32_t(TailStage::Presentation)){
   if(r.receipt){const auto& t=tokens[r.receipt-1];o<<",\"requestedToken\":\""<<t.requested.data()<<"\",\"renderedToken\":\""<<t.rendered.data()<<'"';}
   else o<<",\"tokenEvidence\":\"MISSING_BOUNDED_RECEIPT\"";
  }
 }o<<'}';}o<<"],\"next\":"<<end<<",\"completePage\":true}";return o.str();}
};
}
