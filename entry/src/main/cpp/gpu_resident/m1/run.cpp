#include "run.h"
#include <chrono>
namespace grs {namespace m1 {
ControlCode Run::Offer(const std::array<uint32_t,32>& command){
 if(suspended)return ControlCode::BUSY;
 if(offered==UINT64_MAX)return ControlCode::TOKEN_EXHAUSTED;offered++;
 if(count==queue.size()){backpressured++;return ControlCode::BACKPRESSURE;}
 queue[(head+count)%queue.size()]=command;count++;return ControlCode::OK;
}
ControlCode Run::Advance(uint64_t deltaNs,U64 f,U64 c,int target,uint32_t capacity){if(suspended||receipt.active||inflight||quarantined)return ControlCode::BUSY;
 // dt0 has no writes, including frame serial, carry, queue and receipt.
 if(deltaNs==0)return ControlCode::OK;
 if(Compare64(f,lastFrame)<=0||(f.lo|f.hi)==0||(c.lo|c.hi)==0||target< -7680||target>7680||(capacity!=192&&capacity!=512))return ControlCode::INVALID_INPUT;
 TickPlan plan;auto code=clock.Plan(deltaNs,plan);if(code!=ControlCode::OK){head=count=0;transport.Stop();return code;}
 frame=lastFrame=f;control=c;angle=target;K=capacity;receipt={};receipt.time=plan;receipt.active=true;nextQuantum=1;sourcePending=count!=0;
 if(!sourcePending&&plan.quanta==0){receipt.active=false;receipt.complete=true;}return ControlCode::OK;
}
void Run::Fail(TransportCode code){receipt.code=code;receipt.fault=true;receipt.active=false;receipt.complete=true;receipt.discardedQuanta=receipt.time.quanta-receipt.successfulQuanta;receipt.time.discardedQuanta=receipt.discardedQuanta;}
ControlCode Run::AdvancePlanV2(const PlanGrantV2& grant,U64 f,uint32_t capacity){
 if(suspended||receipt.active||inflight||submitting||quarantined)return ControlCode::BUSY;
 if(grant.id<=planV2||grant.quanta>4||grant.carry>=1000000000||
    grant.angle< -7680||grant.angle>7680||(capacity!=192&&capacity!=512)||
    Compare64(f,lastFrame)<=0)return ControlCode::INVALID_INPUT;
 progressV2=true;planV2=grant.id;frame=lastFrame=f;control={uint32_t(grant.id),uint32_t(grant.id>>32)};
 angle=grant.angle;K=capacity;receipt={};receipt.time.quanta=grant.quanta;
 receipt.time.carryAfter=uint32_t(grant.carry);receipt.active=true;nextQuantum=1;sourcePending=count!=0;
 if(!sourcePending&&grant.quanta==0){receipt.active=false;receipt.complete=true;}
 return ControlCode::OK;
}
void Run::DiscardSourceV2(){
 if(!progressV2){DropRevoked();return;}
 head=count=0;sourcePending=sourceFlight=submitting=inflight=false;
 receipt.active=nextQuantum<=receipt.time.quanta;receipt.complete=!receipt.active;
 receipt.code=TransportCode::REVOKED;
}
void Run::DropRevoked(){
 if(progressV2&&sourceFlight){DiscardSourceV2();return;}
 head=count=0;sourcePending=sourceFlight=submitting=inflight=false;
 receipt.code=TransportCode::REVOKED;receipt.active=false;receipt.complete=true;
 receipt.discardedQuanta=receipt.time.quanta-receipt.successfulQuanta;
 receipt.time.discardedQuanta=receipt.discardedQuanta;
}
TransportCode Run::Pump(){
 if(quarantined){AcceptedRoot ignored;auto code=transport.Poll(ignored);if(code!=TransportCode::BUSY){quarantined=false;inflight=false;}return code;}
 if(!receipt.active)return receipt.fault?receipt.code:TransportCode::NOT_READY;
 // T324 §2.3 + Amendment 1 (T328 ruling): continuation branch, before
 // inflight. Same nonce, no root construction, no Add64 — the transport
 // holds the pending root. The SINGLE Pump now loops ContinueSubmit until
 // the root seals (OK -> inflight) or the per-Pump budget (kPumpBudgetNs)
 // is exhausted; batch boundaries truncate naturally (no half batch).
 // INV-5: per-batch SubmitEnter/Exit telemetry still fires per batch inside
 // the loop (41-dict carry keeps incrementing per batch).
 // INV-7: the first batch stays inside SubmitQuantum; this loop only drives
 // continuations.
 if(submitting){
  // T329 kPumpBudgetNs: single-Pump guard rail (8ms, independent from the
  // batch budget — defensive ceiling so the bridge loop returns on time and
  // the post-PumpEnd GAP check keeps its current frequency, INV-6).
  constexpr uint64_t kPumpBudgetNs=8000000;
  const auto pumpStart=std::chrono::steady_clock::now();
  while(true){
   const auto before=transport.tailObserver.Stamp();
   auto code=transport.ContinueSubmit();
   const auto after=transport.tailObserver.Stamp();
   const auto root=(uint64_t(nonce.hi)<<32)|nonce.lo;
   transport.tailObserver.Emit({TailStage::Continuation,root,before,after,0,0,0,uint32_t(code)});
   if(code==TransportCode::BUSY)return code;
   if(code==TransportCode::OK_SUBMITTING){
    const auto elapsed=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-pumpStart).count());
    if(elapsed<kPumpBudgetNs)continue; // (ii) budget remains: next batch this Pump
    return TransportCode::OK;          // (iii) budget exhausted: next Pump continues
   }
   if(code==TransportCode::REVOKED){DropRevoked();return code;}
   if(code!=TransportCode::OK){Fail(code);return code;}
   inflight=true;submitting=false;
   transport.tailObserver.Emit({TailStage::Inflight,root,after,after,0,0,0,0,2});
   return TransportCode::OK; // (i) sealed
  }
 }
 if(inflight){AcceptedRoot accepted;auto code=transport.Poll(accepted);if(code==TransportCode::BUSY)return code;inflight=false;
  if(code==TransportCode::REVOKED){DropRevoked();return code;}
  if(code!=TransportCode::OK){Fail(code);return code;}receipt.accepted++;receipt.latest=accepted;
  nextPublication=1-accepted.publication;
  if(sourceFlight){head=(head+1)%queue.size();count--;sourcePending=false;}else{receipt.successfulQuanta++;nextQuantum++;}
  if(suspended){receipt.active=false;receipt.complete=true;receipt.discardedQuanta=receipt.time.quanta-receipt.successfulQuanta;return TransportCode::OK;}
  if(!sourcePending&&nextQuantum>receipt.time.quanta){receipt.active=false;receipt.complete=true;}return TransportCode::OK;
 }
 RootRequest root;U64 next;if(!Add64(nonce,{1,0},next)){Fail(TransportCode::PROTOCOL_REJECTED);return receipt.code;}
 root.kind=sourcePending?1:2;root.nonce=next;root.frame=frame;root.control=control;root.carry=receipt.time.carryAfter;root.publicationSlot=uint32_t((next.lo-1)&1u);root.eventArena=frame.lo&1u;root.eventCapacity=0;root.quantumOrdinal=sourcePending?0:nextQuantum;
 if(transport.isolation)root.publicationSlot=nextPublication;
 auto code=sourcePending?transport.SubmitSource(queue[head],root):transport.SubmitQuantum([&]{auto p=params;p.requestedAngle=angle;return p;}(),root);
 // T324 §2.3: OK_SUBMITTING consumes the nonce immediately (no retry can
 // re-Begin the root); the root is unsealed, NOT inflight. Poll/Stop drain
 // semantics below only ever see sealed inflight roots.
 if(code==TransportCode::OK_SUBMITTING){nonce=next;sourceFlight=sourcePending;submitting=true;receipt.submitted++;return TransportCode::OK;}
 if(code==TransportCode::REVOKED){nonce=next;sourceFlight=sourcePending;DropRevoked();return code;}
 if(code==TransportCode::BUSY)return code;if(code!=TransportCode::OK){Fail(code);return code;}
 nonce=next;sourceFlight=sourcePending;inflight=true;receipt.submitted++;
 const auto stamp=transport.tailObserver.Stamp();
 transport.tailObserver.Emit({TailStage::Inflight,(uint64_t(nonce.hi)<<32)|nonce.lo,stamp,stamp,0,0,0,0,root.kind});
 return TransportCode::OK;
}
uint32_t Run::DiscardSources(){uint32_t keep=SourceInFlight()?1:0,dropped=count-keep;count=keep;if(!keep){head=0;sourcePending=false;if(!inflight&&nextQuantum>receipt.time.quanta){receipt.active=false;receipt.complete=true;}}return dropped;}
void Run::Suspend(){DiscardSources();suspended=true;clock.Pause();if(!inflight){receipt.active=false;receipt.complete=true;receipt.discardedQuanta=receipt.time.quanta-receipt.successfulQuanta;}}
void Run::Resume(){suspended=false;clock.Pause();}
ControlCode Run::Pause(){clock.Pause();head=count=0;sourcePending=false;
 // T324 §2.4 drain order: an unsealed root must be CANCELLED, never
 // quarantined (quarantine = sealed fence awaiting signal; submitting has no
 // fence a Stop could drain). CancelSubmit resets the transport to IDLE and
 // the root can never reach inflight/accepted; the run still fails with
 // PROTOCOL_REJECTED exactly like the original Pause.
 if(submitting){transport.CancelSubmit();submitting=false;}
 auto code=transport.Stop();if(inflight){quarantined=true;Fail(TransportCode::PROTOCOL_REJECTED);}else if(receipt.active){Fail(TransportCode::PROTOCOL_REJECTED);}return code;}
}

}
