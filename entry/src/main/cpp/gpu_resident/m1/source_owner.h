#pragma once
#include "../control.h"
#include "profile.h"
namespace grs {namespace m1 {
// A revision2 SOURCE-only owner. It cannot accept M0 or QUANTUM completions.
class SourceRootOwner {
public:
 ControlCode Initialize(const Identity&,uint32_t K,uint32_t Kw);
 ControlCode Begin(const RootRequest&,const std::array<uint32_t,32>&);
 ControlCode Poll(FenceState,const uint32_t*,size_t,uint32_t,AcceptedRoot&);
 ControlCode Stop();void ContextLost();
 ControlCode Acquire(uint32_t,U64,PublicationLease&);ControlCode Release(uint32_t,U64,bool);
 uint32_t Bank()const{return bank;}const Words256& Header()const{return header;}FlightState State()const{return state;}
 uint64_t MappedBytes()const{return mapped;}uint64_t AcceptedCount()const{return accepted;}
 bool HasLeases()const{for(const auto& p:pubs)if(p.leased||p.releasePending)return true;return false;}
private:
 friend class FullM1Transport;
 friend struct FullM1HostAccess;
 ControlCode BeginQuantum(const RootRequest&,int requestedAngle);
 bool ValidQuantum(const uint32_t*,size_t,uint32_t)const;int quantumAngle=0;
 bool fullWaterContract=false;
 bool Valid(const uint32_t*,size_t,uint32_t)const;
 Words256 header{};Identity identity{};RootRequest pending{};std::array<uint32_t,32> command{};
 PublicationLease pubs[2]{};FlightState state=FlightState::StateLost;uint32_t K=0,Kw=0,bank=0;U64 lastNonce{};bool tombstone=false;uint64_t mapped=0,accepted=0;
};
}}
