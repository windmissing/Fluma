#include "../../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h"
#include <cassert>
#include <iostream>
#include <cstring>
using namespace grs::m1;
int main(){
 ProgressClockV2 clock;
 assert(clock.BeginSession());auto session=clock.session;
 assert(!clock.Fresh(1));assert(!strcmp(clock.Pulse(session,1,100,100),"PAGE_PULSE_ACCEPTED"));
 clock.ready=true;
 assert(!strcmp(clock.Pulse(session,2,101,101),"PAGE_PULSE_ACCEPTED"));
 uint64_t n=101,seq=2,oracleGenerated=0,oracleCarry=0;
 for(unsigned i=0;i<100000;i++){
  const uint64_t delta=(i%3==0)?16000000:8333333;
  n+=delta;
  const auto scaled=oracleCarry+120*delta;
  oracleGenerated+=scaled/1000000000;oracleCarry=scaled%1000000000;
  assert(!strcmp(clock.Pulse(session,++seq,n,n),"PAGE_PULSE_ACCEPTED"));
  assert(clock.generated==oracleGenerated&&clock.carry==oracleCarry&&clock.Balanced()&&clock.pending<=4);
  if(i%5==0&&!clock.activeRemaining){
   PlanGrantV2 grant;assert(clock.Claim(n,0,1,false,grant));assert(grant.quanta<=4);
  }
  if(i%4==0&&clock.activeRemaining)assert(clock.AcceptQuantum(clock.active.id));
 }
 assert(clock.overload>0);clock.Revoke("CLOCK_GAP");assert(clock.Balanced()&&clock.carry==0);
 assert(clock.BeginSession());session=clock.session;clock.ResetRun();
 assert(!strcmp(clock.Pulse(session,1,1,1),"PAGE_PULSE_ACCEPTED"));
 assert(!strcmp(clock.Pulse(session,1,2,2),"PULSE_SEQUENCE_REJECTED"));
 assert(!strcmp(clock.Pulse(session-1,2,2,2),"STALE_PAGE_SESSION"));
 assert(!strcmp(clock.Pulse(session,2,33333335,2),"CLOCK_GAP"));
 assert(!clock.open);assert(clock.BeginSession());session=clock.session;
 assert(!strcmp(clock.Pulse(session,1,1,1),"PAGE_PULSE_ACCEPTED"));
 assert(!strcmp(clock.Pulse(session,2,2,33333335),"CLOCK_GAP"));
 assert(clock.Balanced());
 // Exact carry-to-quota boundary, reset ledger and exhaustion rejection.
 assert(clock.BeginSession());session=clock.session;clock.ResetRun();clock.ready=true;
 assert(!strcmp(clock.Pulse(session,1,100,100),"PAGE_PULSE_ACCEPTED"));
 clock.carry=999999880;
 assert(!strcmp(clock.Pulse(session,2,101,101),"PAGE_PULSE_ACCEPTED"));
 assert(clock.generated==1&&clock.pending==1&&clock.carry==0&&clock.Balanced());
 PlanGrantV2 grant;assert(clock.Claim(101,0,1,false,grant));
 assert(!clock.AcceptQuantum(grant.id+1));assert(clock.AcceptQuantum(grant.id));
 assert(!clock.AcceptQuantum(grant.id));assert(clock.Balanced());
 clock.ResetRun();assert(clock.Balanced()&&clock.generated==0&&clock.pending==0);
 clock.ready=true;clock.baseline=true;clock.generated=clock.accepted=UINT64_MAX-1;
 clock.carry=999999999;
 assert(!strcmp(clock.Pulse(session,3,33333434,33333434),"QUOTA_OVERFLOW"));
 assert(!clock.open&&clock.Balanced());
 assert(clock.BeginSession());clock.planSerial=UINT64_MAX;clock.ready=true;clock.hasPulse=true;
 clock.receiveNs=1;assert(!clock.Claim(1,0,1,true,grant));
 clock.session=UINT64_MAX;assert(!clock.BeginSession());
 std::cout<<"T404 exact integer quota oracle 100000 pulses, overload, carry, revoke, dual-domain GAP PASS\n";
}
