#pragma once
#include "control.h"
#include <array>
#include <algorithm>
#include <cstring>
#include <ostream>
#include <cstdint>
namespace grs::recovery467 {
enum class Phase {FAULTED_REVOKED,RESET_REQUESTED,RETIRING_OLD,CREATING_NEW,WAIT_NEW_SESSION,READY};
inline const char* Name(Phase p){static const char* names[]={"FAULTED_REVOKED","RESET_REQUESTED","RETIRING_OLD","CREATING_NEW","WAIT_NEW_SESSION","READY"};return names[int(p)];}
struct Event {const char* kind="";uint64_t stamp=0,tid=0,serial=0,epoch=0,session=0,revision=0,root=0,a=0,b=0,c=0;uint32_t role=0,code=0;};
struct Ledger {uint64_t epoch=0,session=0,generated=0,accepted=0,overload=0,revoked=0,carry=0;};
// The bridge permit mutex owns this state; no GL or waiting in this helper.
struct Transaction {
 Phase phase=Phase::FAULTED_REVOKED;uint64_t serial=0,epoch=0,session=0,pageGeneration=0;
 Identity candidate{};void* window=nullptr;bool retired=false,resources=false;
 const char* reason="WAIT_EXPLICIT_RESET";const char* firstReason="NONE";
 std::array<Event,512> events{};size_t count=0;uint64_t dropped=0;
 std::array<Ledger,8> ledgers{};uint64_t ledgersSeen=0;
 Event inCall{};bool calling=false;
 void Note(Event e){e.serial=serial;e.epoch=epoch;e.session=session;if(count<events.size())events[count++]=e;else ++dropped;}
 void Fault(const char* why){phase=Phase::FAULTED_REVOKED;resources=false;reason=why;if(std::strcmp(firstReason,"NONE")==0)firstReason=why;}
 bool Request(uint64_t e,void* w){
  if(serial==UINT64_MAX||e==0){Fault("RESET_ID_OVERFLOW");return false;}
  ++serial;epoch=e;window=w;session=pageGeneration=0;candidate={};retired=resources=false;
  phase=Phase::RESET_REQUESTED;reason="WAIT_RETIRED";return true;
 }
 bool Retiring(uint64_t e){if(e!=epoch||phase==Phase::FAULTED_REVOKED)return false;phase=Phase::RETIRING_OLD;return true;}
 bool Retired(uint64_t e){if(e!=epoch||phase==Phase::FAULTED_REVOKED)return false;retired=true;phase=Phase::CREATING_NEW;reason="CREATING_NEW";return true;}
 bool Resources(uint64_t e,void* w,const Identity& id){
  if(e!=epoch||w!=window||!retired||phase!=Phase::CREATING_NEW||!(id.run.lo|id.run.hi))return false;
  candidate=id;resources=true;phase=Phase::WAIT_NEW_SESSION;reason="WAIT_NEW_SESSION";return true;
 }
 bool Match(uint64_t s,uint64_t e,U64 run,void* w)const{return s==serial&&e==epoch&&w==window&&Compare64(run,candidate.run)==0;}
 bool CanSession(uint64_t s,uint64_t e,U64 run,void* w)const{return Match(s,e,run,w)&&resources&&retired&&phase==Phase::WAIT_NEW_SESSION&&session==0;}
 bool Bind(uint64_t s,uint64_t generation){if(!s||!generation||phase!=Phase::WAIT_NEW_SESSION||session)return false;session=s;pageGeneration=generation;return true;}
 bool CanReady(uint64_t e,void* w,const Identity& id,uint64_t token)const{
  return CanScope(e,w,id)&&phase==Phase::WAIT_NEW_SESSION&&session&&token==session;
 }
 bool CanScope(uint64_t e,void* w,const Identity& id)const{return resources&&retired&&e==epoch&&w==window&&Compare64(id.run,candidate.run)==0&&Compare64(id.context,candidate.context)==0&&Compare64(id.surface,candidate.surface)==0;}
 void Ready(){phase=Phase::READY;reason="READY";}
 void Archive(const Ledger& l){if(l.epoch)ledgers[ledgersSeen++%ledgers.size()]=l;}
 void Json(std::ostream& o,bool full)const{
  o<<"{\"revision\":\"T464_RECOVERY_V1\",\"phase\":\""<<Name(phase)<<"\",\"reason\":\""<<reason<<"\",\"firstReason\":\""<<firstReason
   <<"\",\"resetSerial\":\""<<serial<<"\",\"epoch\":\""<<epoch<<"\",\"session\":\""<<session<<"\",\"runLo\":"<<candidate.run.lo<<",\"runHi\":"<<candidate.run.hi
   <<",\"contextLo\":"<<candidate.context.lo<<",\"contextHi\":"<<candidate.context.hi<<",\"retired\":"<<(retired?"true":"false")<<",\"resources\":"<<(resources?"true":"false");
  if(full){o<<",\"incomplete\":"<<(dropped?"true":"false")<<",\"dropped\":"<<dropped<<",\"inCall\":"<<(calling?"true":"false")<<",\"callRoot\":\""<<inCall.root<<"\",\"callEnterNs\":\""<<inCall.stamp<<"\",\"callFrom\":"<<inCall.a<<",\"callTo\":"<<inCall.b<<",\"events\":[";
   for(size_t i=0;i<count;++i){const auto& e=events[i];if(i)o<<',';o<<"{\"kind\":\""<<e.kind<<"\",\"ns\":\""<<e.stamp<<"\",\"tid\":\""<<e.tid<<"\",\"role\":"<<e.role<<",\"serial\":\""<<e.serial<<"\",\"epoch\":\""<<e.epoch<<"\",\"session\":\""<<e.session<<"\",\"permissionRevision\":\""<<e.revision<<"\",\"root\":\""<<e.root<<"\",\"a\":\""<<e.a<<"\",\"b\":\""<<e.b<<"\",\"c\":\""<<e.c<<"\",\"code\":"<<e.code<<'}';}
   o<<"],\"oldLedgers\":[";for(uint64_t i=0;i<std::min<uint64_t>(ledgersSeen,8);++i){const auto& l=ledgers[(ledgersSeen-std::min<uint64_t>(ledgersSeen,8)+i)%8];if(i)o<<',';o<<"{\"epoch\":\""<<l.epoch<<"\",\"session\":\""<<l.session<<"\",\"generated\":\""<<l.generated<<"\",\"accepted\":\""<<l.accepted<<"\",\"overload\":\""<<l.overload<<"\",\"revoked\":\""<<l.revoked<<"\",\"clearedCarry\":\""<<l.carry<<"\"}";}o<<']';
  }o<<'}';
 }
};
}
