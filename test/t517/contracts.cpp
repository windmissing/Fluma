#include "m3b/lineage_schema.h"
#include "m3b/compound_cursor.h"
#include "m3b/lift_input.h"
#include <cassert>
#include <iostream>
using namespace grs::m3b;
int main(){
 static_assert(input64::reserved63==63&&certificate16::reserved15==15);
 static_assert(header256::liftPolicy==161&&completion256::liftPolicy==189);
 for(const auto& p:kLineageProfiles){
  assert(p.stateBytes==p.oldStateBytes+131584+80*p.k);
  assert(p.e==48*p.k+16*p.kw+510&&p.ea==127*64+p.k+257);
  assert(p.rootBytes==32768+p.stateBytes+10224000+512*p.sourceRows+256*(p.k+4*p.e)+128*4*p.ea+8192);
  assert(p.fullBytes==p.rootBytes+5*(p.stateBytes+10224000)+4*8192+20*(256*p.e+128*p.ea));
  assert(2*p.fullBytes<=kColdCapacity);
  size_t at=p.oldStateBytes;for(const auto& s:p.extension){assert(s.begin==at&&s.begin%16==0&&s.bytes%16==0);at=s.End();}assert(at==p.stateBytes);
 }
 CompoundCursor c;assert(c.Reserve(1,64,2,64));assert(!c.Complete());assert(c.sealedPhysical==0&&c.sealedAux==0);
 assert(!c.Reserve(1,0,2,64));c.Fail(39);assert(!c.Seal()&&c.plannedPhysical==1&&c.sealedPhysical==0&&c.firstFailure==39);
 c={};assert(c.Reserve(1,2,1,2)&&c.Seal()&&c.Complete());assert(!c.Reserve(1,0,1,2));
 c={};assert(!c.Reserve(0,0,0,0));c.plannedPhysical=c.sealedPhysical=UINT32_MAX;assert(!c.Reserve(0,1,UINT32_MAX,1));
 LiftParameters parameters;parameters.mode=LiftMode::SYNTHETIC_TEST;parameters.revision=1;parameters.sha[0]=1;
 parameters.bandQ=1048576;parameters.rampQ16=65536;parameters.decayQ16=65536;parameters.liftSpeedQ=1048576;parameters.liftCapQ=1048576;
 LiftInputGate gate;LiftInputIdentity identity{1,2,3,4,5};assert(gate.Configure(identity,parameters,true)==InputResult::OK);
 LiftInput64 raw{};raw[0]=893994572;raw[1]=5;raw[2]=2;raw[6]=1;raw[7]=1;raw[14]=2;raw[15]=1;raw[48]=1;
 InputPut64(raw,4,1);InputPut64(raw,8,100);InputPut64(raw,40,2);InputPut64(raw,42,3);InputPut64(raw,44,4);InputPut64(raw,46,5);
 assert(gate.Offer(raw,250000100)==InputResult::OK);const LiftInput64* frozen=nullptr;
 assert(gate.Freeze(1,250000100,frozen)==InputResult::OK&&frozen&&InputU64(*frozen,12)==250000100);
 assert(gate.Freeze(1,250000101,frozen)==InputResult::STALE_INPUT&&!frozen);
 raw[6]=2;InputPut64(raw,8,300000000);assert(gate.Offer(raw,299999999)==InputResult::STALE_INPUT);
 assert(gate.Offer(raw,300000000)==InputResult::OK);assert(gate.Freeze(2,300000000,frozen)==InputResult::OK);
 raw[6]=3;InputPut64(raw,8,300000001);assert(gate.Offer(raw,300000001)==InputResult::OK);
 assert(gate.Freeze(2,300000002,frozen)==InputResult::OK&&(*frozen)[6]==2); // no mid-plan replacement
 InputPut64(raw,10,1);assert(gate.Offer(raw,300000002)==InputResult::BAD_ENVELOPE);InputPut64(raw,10,0);
 parameters.mode=LiftMode::SENSOR_RESEARCH;assert(gate.Configure(identity,parameters,false)==InputResult::OK);
 raw[2]=1;raw[14]=1;raw[3]=3;assert(gate.Offer(raw,300000002)==InputResult::SENSOR_CLOCK_UNVERIFIED);
 parameters.mode=LiftMode::OFF;assert(gate.Configure(identity,parameters,false)==InputResult::OK);
 assert(gate.Freeze(1,UINT64_MAX,frozen)==InputResult::OK);assert((*frozen)[2]==0&&InputU64(*frozen,8)==0&&InputU64(*frozen,12)==0);
 std::cout<<"T517 V5 four-profile ABI, exact cold capacities and paired prefix checks PASS\n";
}
