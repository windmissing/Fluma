#pragma once
#include <cstdint>
#include <limits>
namespace grs {namespace m1 {
struct PlanGrantV2 {
 uint64_t id=0,grantedNs=0,carry=0,session=0,sourceBoundary=0;
 uint32_t quanta=0;
 int angle=0;
};
class ProgressClockV2 {
public:
 static constexpr uint64_t GapNs=33333333,JobNs=250000000,Denominator=1000000000;
 uint64_t session=0,sequence=0,uiNs=0,receiveNs=0,pulses=0,maxNativeGap=0,maxUiGap=0;
 uint64_t generated=0,accepted=0,overload=0,revoked=0,carry=0,clearedCarry=0,planSerial=0;
 uint64_t droppedSources=0,lateSupervisorNs=0;
 uint32_t pending=0,activeRemaining=0;
 bool open=false,ready=false,baseline=false,hasPulse=false;
 const char* fault="WAIT_PAGE_PULSE";
 PlanGrantV2 active{};
 bool BeginSession(){
  if(session==UINT64_MAX)return false;
  Revoke("SESSION_REPLACED");++session;sequence=uiNs=receiveNs=0;
  open=true;hasPulse=false;fault="WAIT_PAGE_PULSE";return true;
 }
 void Revoke(const char* why){
  revoked+=uint64_t(pending)+activeRemaining;pending=activeRemaining=0;
  clearedCarry+=carry;carry=0;baseline=false;ready=false;open=false;fault=why;
 }
 void ResetRun(){
  generated=accepted=overload=revoked=carry=clearedCarry=0;
  pending=activeRemaining=0;baseline=ready=false;active={};
  fault="WAIT_PAGE_PULSE";
 }
 bool Fresh(uint64_t now){
  if(!open)return false;
  if(hasPulse&&(now<receiveNs||now-receiveNs>GapNs)){
   if(now>=receiveNs)lateSupervisorNs=now-receiveNs-GapNs;
   Revoke(now<receiveNs?"CONTROL_CLOCK_REVERSED":"CLOCK_GAP");return false;
  }
  return hasPulse;
 }
 const char* Pulse(uint64_t token,uint64_t seq,uint64_t callback,uint64_t now){
  if(token!=session||!open)return "STALE_PAGE_SESSION";
  if(seq==0||seq<=sequence)return "PULSE_SEQUENCE_REJECTED";
  if(hasPulse){
   if(callback<=uiNs||now<=receiveNs){Revoke("PULSE_CLOCK_REVERSED");return fault;}
   const auto nativeGap=now-receiveNs,uiGap=callback-uiNs;
   if(nativeGap>maxNativeGap)maxNativeGap=nativeGap;
   if(uiGap>maxUiGap)maxUiGap=uiGap;
   if(nativeGap>GapNs||uiGap>GapNs){Revoke("CLOCK_GAP");return fault;}
   if(ready&&baseline){
    const uint64_t scaled=carry+120*nativeGap;
    const uint64_t fresh=scaled/Denominator;
    carry=scaled%Denominator;
    if(generated>UINT64_MAX-fresh||overload>UINT64_MAX-fresh){Revoke("QUOTA_OVERFLOW");return fault;}
    generated+=fresh;
    const uint64_t keep=fresh<4-pending?fresh:4-pending;
    pending+=uint32_t(keep);overload+=fresh-keep;
   }else if(ready)baseline=true;
  }else if(ready)baseline=true;
  sequence=seq;uiNs=callback;receiveNs=now;hasPulse=true;++pulses;
  return "PAGE_PULSE_ACCEPTED";
 }
 bool Claim(uint64_t now,int angle,uint64_t boundary,bool source,PlanGrantV2& out){
  if(!ready||!Fresh(now)||activeRemaining||(!pending&&!source)||planSerial==UINT64_MAX)return false;
  active={++planSerial,now,carry,session,boundary,pending,angle};
  activeRemaining=pending;pending=0;out=active;return true;
 }
 bool AcceptQuantum(uint64_t plan){
  if(plan!=active.id||activeRemaining==0||accepted==UINT64_MAX)return false;
  ++accepted;--activeRemaining;return true;
 }
 bool Identity(uint64_t plan,uint64_t token)const{return open&&ready&&plan==active.id&&token==session;}
 bool Balanced()const{return generated==accepted+activeRemaining+pending+overload+revoked;}
};
}}
