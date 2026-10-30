#pragma once
#include "bridge_packet.h"
#include <atomic>
#include <mutex>
#include <string>
namespace grs {
// All methods ending Locked require the same mutex. EGL initialization stays
// local until CommitReadyLocked atomically publishes permission and identity.
struct BridgeState {
 std::mutex mutex;void* window=nullptr;std::atomic<uint64_t> epoch{1};bool active=true;uint32_t K=192;int angle=0;
 struct Command {std::array<uint32_t,32> words{};uint64_t epoch=0;U64 input{};};
 std::array<Command,16> queue{};size_t head=0,count=0;bool ready=false;uint64_t readyEpoch=0,cancelled=0,staleRejected=0;
 Identity identity{};U64 input{};uint32_t runQueue=0;const char* phase="WAIT_WINDOW";const char* permission="WAIT_WINDOW";
 std::string status="{\"state\":\"WAIT_WINDOW\"}";
 bool CurrentLocked(uint64_t seen,void* held)const{return seen==epoch.load()&&active&&held&&held==window;}
 bool AllowedLocked()const{return ready&&readyEpoch==epoch.load()&&active&&window&&std::strcmp(permission,"READY")==0;}
 void RevokeLocked(const char* why){ready=false;readyEpoch=0;permission=why;cancelled+=count;head=count=0;}
 void InvalidateLocked(){++epoch;RevokeLocked("STATE_LOST");runQueue=0;phase="STATE_LOST";status="{\"state\":\"STATE_LOST\",\"newRunRequired\":true}";}
 bool CommitReadyLocked(uint64_t seen,void* held,const Identity& id,U64 generation){if(!CurrentLocked(seen,held))return false;identity=id;input=generation;readyEpoch=seen;permission=phase="READY";ready=true;runQueue=0;status="{\"state\":\"READY\"}";return true;}
 const char* OfferLocked(const std::array<uint32_t,32>& w){if(!AllowedLocked())return "NOT_READY";auto result=ValidateBridgePacket(w,identity,input);if(std::strcmp(result,"OK")!=0)return result;if(count==queue.size())return "BACKPRESSURE";queue[(head+count)%queue.size()]={w,epoch.load(),input};++count;return "ENQUEUED_NOT_ACK";}
 bool FrontLocked(uint64_t seen,U64 actualInput,std::array<uint32_t,32>& out){if(!AllowedLocked()||seen!=epoch.load()||!count)return false;const auto& q=queue[head];if(q.epoch!=seen||q.input.lo!=actualInput.lo||q.input.hi!=actualInput.hi||std::strcmp(ValidateBridgePacket(q.words,identity,actualInput),"OK")!=0){++staleRejected;++cancelled;PopLocked();return false;}out=q.words;return true;}
 void PopLocked(){head=(head+1)%queue.size();--count;}
};
}
