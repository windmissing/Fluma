// T236 native control plane only. No CPU inventory, integration or budget solver.
#pragma once
#include "layout_generated.h"
#include "integer.h"
#include <array>
namespace grs {
using Words256=std::array<uint32_t,256>;
enum class ControlCode {OK,UNSUPPORTED_FEATURE,INVALID_INPUT,BUSY,BACKPRESSURE,STALE,QUARANTINED,OWNER_STATE_LOST,COMPLETION_INVALID,TOKEN_EXHAUSTED,CAPABILITY_REQUIRED};
enum class FenceState {Pending,Signaled,Failed,Timeout};
enum class FlightState {Idle,Submitted,Quarantined,StateLost};
struct TickPlan {uint32_t quanta=0,carryBefore=0,carryAfter=0;uint64_t deltaNs=0,discardedQuanta=0;};
class Fixed120Clock {
 public:ControlCode Plan(uint64_t deltaNs,TickPlan&);void Pause(){carry=0;first=true;}uint32_t Carry()const{return carry;}
 private:uint32_t carry=0;bool first=true;
};
struct Identity {U64 run,owner,context,surface,geometry,config;std::array<uint32_t,8> shaderSha{},angleSha{},layoutSha{};};
struct RootRequest {uint32_t kind=0,quantumOrdinal=0;U64 nonce,control,frame;uint32_t publicationSlot=0,eventArena=0,eventStart=0,eventCapacity=0;uint32_t dispatchCount=0,barrierCount=0,uploadedBytes=0;uint32_t carry=0;};
struct CompletionPolicy {
 // No guessed V1 wire enums: production integrator must supply architect-defined codes.
 uint32_t magic=0,revision=0,seal=0,successOutcome=0,metadataRejectOutcome=0,replayOutcome=0;
 uint32_t sourceKind=0,quantumKind=0,successReceipt=0,partialReceipt=0,rejectedReceipt=0;
 bool productWire=false;U64 requiredStages,requiredAudit;uint32_t replayEqualSentinel=0;
};
CompletionPolicy ProductCompletionPolicy();
struct AcceptedRoot {uint32_t bank=0,publication=0,outcome=0;U64 source,physics,publicationGeneration,nonce;bool sourceSuccess=false;Words256 completion{};};
struct PublicationLease {Identity identity;U64 source,physics,control,frame,nonce;uint32_t angle=0;bool available=false,leased=false,releasePending=false;U64 generation,run,context,surface;uint32_t bank=0;};
class RootOwner {
 public:
 ControlCode Initialize(const Identity&,const CompletionPolicy&,const Words256& coldHeader,uint32_t features);
 ControlCode Begin(const RootRequest&);
 ControlCode Poll(FenceState,const uint32_t* completion,size_t words,uint32_t rawGlError,AcceptedRoot&);
 ControlCode Stop();void ContextLost();ControlCode ResetWhenRetired();
 ControlCode Acquire(uint32_t slot,U64 generation,PublicationLease& out);
 ControlCode Release(uint32_t slot,U64 generation,bool fenceSignaled);
 ControlCode RetireRelease(uint32_t slot,U64 generation,bool fenceSignaled);
 FlightState State()const{return state;}uint32_t Bank()const{return acceptedBank;}
 const Words256& Header()const{return header;}const RootRequest& Pending()const{return pending;}
 uint64_t MappedBytes()const{return mappedBytes;}uint64_t AcceptedCount()const{return acceptedCount;}
 private:
 bool ValidCompletion(const uint32_t*,size_t,uint32_t)const;bool Match(const uint32_t*,size_t,U64)const;
 Words256 header{};Identity identity;CompletionPolicy policy;RootRequest pending;FlightState state=FlightState::StateLost;
 uint32_t acceptedBank=0,candidateBank=1,spareBank=2;bool tombstone=false;U64 lastNonce{};
 PublicationLease publications[2];uint64_t mappedBytes=0,acceptedCount=0;
};
}
