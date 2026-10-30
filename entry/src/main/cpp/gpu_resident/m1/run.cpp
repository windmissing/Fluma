#include "run.h"
namespace grs {namespace m1 {
ControlCode Run::Offer(const std::array<uint32_t,32>& command){
 if(suspended)return ControlCode::BUSY;
 if(offered==UINT64_MAX)return ControlCode::TOKEN_EXHAUSTED;offered++;
 if(count==queue.size()){backpressured++;return ControlCode::BACKPRESSURE;}
 queue[(head+count)%queue.size()]=command;count++;return ControlCode::OK;
}
ControlCode Run::Advance(uint64_t deltaNs,U64 f,U64 c,int target,uint32_t capacity){
 if(suspended||receipt.active||inflight||quarantined)return ControlCode::BUSY;
 // dt0 has no writes, including frame serial, carry, queue and receipt.
 if(deltaNs==0)return ControlCode::OK;
 if(Compare64(f,lastFrame)<=0||(f.lo|f.hi)==0||(c.lo|c.hi)==0||target< -7680||target>7680||(capacity!=192&&capacity!=512))return ControlCode::INVALID_INPUT;
 TickPlan plan;auto code=clock.Plan(deltaNs,plan);if(code!=ControlCode::OK){head=count=0;transport.Stop();return code;}
 frame=lastFrame=f;control=c;angle=target;K=capacity;receipt={};receipt.time=plan;receipt.active=true;nextQuantum=1;sourcePending=count!=0;
 if(!sourcePending&&plan.quanta==0){receipt.active=false;receipt.complete=true;}return ControlCode::OK;
}
void Run::Fail(TransportCode code){receipt.code=code;receipt.fault=true;receipt.active=false;receipt.complete=true;receipt.discardedQuanta=receipt.time.quanta-receipt.successfulQuanta;receipt.time.discardedQuanta=receipt.discardedQuanta;}
TransportCode Run::Pump(){
 if(quarantined){AcceptedRoot ignored;auto code=transport.Poll(ignored);if(code!=TransportCode::BUSY){quarantined=false;inflight=false;}return code;}
 if(!receipt.active)return receipt.fault?receipt.code:TransportCode::NOT_READY;
 if(inflight){AcceptedRoot accepted;auto code=transport.Poll(accepted);if(code==TransportCode::BUSY)return code;inflight=false;
  if(code!=TransportCode::OK){Fail(code);return code;}receipt.accepted++;receipt.latest=accepted;
  if(sourceFlight){head=(head+1)%queue.size();count--;sourcePending=false;}else{receipt.successfulQuanta++;nextQuantum++;}
  if(suspended){receipt.active=false;receipt.complete=true;receipt.discardedQuanta=receipt.time.quanta-receipt.successfulQuanta;return TransportCode::OK;}
  if(!sourcePending&&nextQuantum>receipt.time.quanta){receipt.active=false;receipt.complete=true;}return TransportCode::OK;
 }
 RootRequest root;U64 next;if(!Add64(nonce,{1,0},next)){Fail(TransportCode::PROTOCOL_REJECTED);return receipt.code;}
 root.kind=sourcePending?1:2;root.nonce=next;root.frame=frame;root.control=control;root.carry=receipt.time.carryAfter;root.publicationSlot=uint32_t((next.lo-1)&1u);root.eventArena=frame.lo&1u;root.eventCapacity=0;root.quantumOrdinal=sourcePending?0:nextQuantum;
 auto code=sourcePending?transport.SubmitSource(queue[head],root):transport.SubmitQuantum([&]{auto p=params;p.requestedAngle=angle;return p;}(),root);
 if(code==TransportCode::BUSY)return code;if(code!=TransportCode::OK){Fail(code);return code;}
 nonce=next;sourceFlight=sourcePending;inflight=true;receipt.submitted++;return TransportCode::OK;
}
uint32_t Run::DiscardSources(){uint32_t keep=SourceInFlight()?1:0,dropped=count-keep;count=keep;if(!keep){head=0;sourcePending=false;if(!inflight&&nextQuantum>receipt.time.quanta){receipt.active=false;receipt.complete=true;}}return dropped;}
void Run::Suspend(){DiscardSources();suspended=true;clock.Pause();if(!inflight){receipt.active=false;receipt.complete=true;receipt.discardedQuanta=receipt.time.quanta-receipt.successfulQuanta;}}
void Run::Resume(){suspended=false;clock.Pause();}
ControlCode Run::Pause(){clock.Pause();head=count=0;sourcePending=false;auto code=transport.Stop();if(inflight){quarantined=true;Fail(TransportCode::PROTOCOL_REJECTED);}else if(receipt.active){Fail(TransportCode::PROTOCOL_REJECTED);}return code;}
}

}
