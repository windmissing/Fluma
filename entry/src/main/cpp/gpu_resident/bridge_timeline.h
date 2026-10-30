#pragma once
#include "control.h"
#include <chrono>
#include <thread>
#include <sstream>
namespace grs {
enum class BridgeStage:uint32_t {Invalidate=1,InitBegin,Ready,ReadyRejected,WaitEnter,Wake,FirstTick,AdvanceBegin,AdvanceEnd,AnchorSet,PumpBegin,PumpEnd,DrawBegin,DrawEnd,SwapBegin,SwapEnd,PauseBegin,PauseEnd,Offer,Consume,Stale,RootSubmit,RootPoll};
struct BridgeTimeRow {uint64_t serial=0,ns=0,tid=0,epoch=0,run=0,surface=0,input=0,now=0,last=0,delta=0,root=0;uint32_t stage=0,code=0,carry=0,quanta=0,appQueue=0,runQueue=0,flags=0;};
struct BridgeTimeline {
 static constexpr uint32_t capacity=1024,pageRows=4;std::array<BridgeTimeRow,capacity> rows{};uint64_t serial=0,dropped=0;uint32_t count=0;
 static uint64_t Now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
 void Add(BridgeTimeRow row){row.serial=++serial;if(count==capacity){++dropped;return;}rows[count++]=row;}
 // Called under App's same lock; explicit bounded cold page, no hot JSON.
 std::string Export(uint32_t cursor)const{std::ostringstream o;o<<"{\"revision\":\"T295_STEADY_TRACE_V1\",\"clock\":\"STEADY_NS\",\"capacity\":"<<capacity<<",\"rowBytes\":"<<sizeof(BridgeTimeRow)<<",\"totalBufferBytes\":"<<sizeof(rows)<<",\"jsonExportBudgetBytes\":1048576,\"count\":"<<count<<",\"stageRevision\":\"T295_STAGE23_V1\",\"serial\":\""<<serial<<"\",\"dropped\":\""<<dropped<<"\""<<",\"incomplete\":"<<(dropped?"true":"false")<<",\"cursor\":"<<cursor<<",\"rows\":[";uint32_t end=std::min(count,cursor+pageRows);for(uint32_t i=cursor;i<end;i++){const auto& r=rows[i];if(i!=cursor)o<<',';o<<"{\"serial\":\""<<r.serial<<"\",\"ns\":\""<<r.ns<<"\",\"tid\":\""<<r.tid<<"\",\"epoch\":\""<<r.epoch<<"\",\"run\":\""<<r.run<<"\",\"surface\":\""<<r.surface<<"\",\"input\":\""<<r.input<<"\",\"now\":\""<<r.now<<"\",\"last\":\""<<r.last<<"\",\"delta\":\""<<r.delta<<"\",\"root\":\""<<r.root<<"\",\"stage\":"<<r.stage<<",\"code\":"<<r.code<<",\"carry\":"<<r.carry<<",\"quanta\":"<<r.quanta<<",\"appQueue\":"<<r.appQueue<<",\"runQueue\":"<<r.runQueue<<",\"flags\":"<<r.flags<<'}';}o<<"],\"next\":"<<end<<",\"completePage\":true}";return o.str();}
};
}
