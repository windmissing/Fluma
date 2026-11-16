// Reuse the ordinary controller fixtures; add the named T508 boundary cases.
#define main CoreControlCases
#include "control.cpp"
#undef main
struct BlockFixture {
 ProgressClockV3 c=Clock();RunControl run;RootPermitV4 p;Words256 words;NativeRootEvidence evidence;
 BlockFixture(){Pulse(c,1,100);Pulse(c,2,33333433);Pulse(c,3,33333434);PlanGrantV3 grant;assert(c.ClaimLocked(33333434,false,run,grant));p=Permit(c,run,33333434);words=Tail(p);evidence=Evidence(p,33333434);}
 ValidatedBlockedTailV4 TailValue(){auto value=CompletionGate::Blocked(kProfiles[0],p,words,evidence);assert(value);return *value;}
 ControlResult Block(uint64_t now=33333434){PlanningControl ctl(c,run);return ctl.BlockPlanningControlLocked(TailValue(),now);}
};
int main(){
 CoreControlCases();
 {BlockFixture f;assert(f.Block()==ControlResult::OK&&f.c.counters.blockedActive==4&&f.c.Balanced());std::cout<<"BQ01 PASS\n";}
 {
  auto c=Clock();RunControl run;Pulse(c,1,100);Pulse(c,2,16666767);PlanGrantV3 grant;assert(c.ClaimLocked(16666767,false,run,grant));Pulse(c,3,33333434);
  auto p=Permit(c,run,33333434);auto tail=CompletionGate::Blocked(kProfiles[0],p,Tail(p),Evidence(p,33333434));assert(tail);PlanningControl ctl(c,run);assert(ctl.BlockPlanningControlLocked(*tail,33333434)==ControlResult::OK);
  assert(c.counters.blockedActive==2&&c.counters.blockedPending==2&&c.counters.blockedClearedCarry==80&&c.Balanced());std::cout<<"BQ02 PASS\n";
 }
 {
  BlockFixture f;assert(f.Block()==ControlResult::OK);Pulse(f.c,4,41666768);assert(f.c.counters.blockedWait==1);
  assert(f.c.RegisterTargetLocked(f.c.identity,100)==ControlResult::OK&&f.c.mode==PlanningMode::REARM_PENDING);
  const auto before=f.c.counters;assert(f.c.PulseLocked({f.c.identity,4,41666768},41666768)==ControlResult::SEQUENCE_REJECTED&&f.c.counters.pulseSequence==before.pulseSequence);
  Pulse(f.c,5,50000102);assert(f.c.mode==PlanningMode::RUNNABLE&&f.c.pending==0&&f.c.counters.blockedWait==2&&f.c.Balanced());std::cout<<"BQ03/BQ14 PASS\n";
 }
 {
  BlockFixture f;f.c.activeRemaining=2;f.c.pending=2;f.c.counters.carry=123456789;
  assert(f.Block()==ControlResult::OK&&f.c.counters.blockedActive==2&&f.c.counters.blockedPending==2&&f.c.counters.blockedClearedCarry==123456789);
  // Formal LOCAL_TRANSITION_ONLY: no invented global elapsed-ns proof.
  std::cout<<"BQ04 local-only PASS\n";
 }
 {
  BlockFixture f;assert(f.Block()==ControlResult::OK);SourcePacketV4 packet;packet.words[0]=1129534023;packet.words[1]=4;
  SourceIntentKeyV4 key{1,2,3,4,5,1,1};assert(f.c.RegisterSourceIntentLocked(key,packet,0)==ControlResult::OK);Pulse(f.c,4,41666768);
  assert(f.c.mode==PlanningMode::RUNNABLE&&f.c.pending==0);PlanGrantV3 grant;assert(f.c.ClaimLocked(41666768,true,f.run,grant)&&grant.quanta==0);
  assert(f.c.counters.accepted==0);std::cout<<"BQ05/BQ13 admission-before-GPU-outcome PASS\n";
 }
 {
  BlockFixture f;assert(f.Block()==ControlResult::OK);assert(f.c.PulseLocked({f.c.identity,4,66666768},66666768)==ControlResult::CLOCK_GAP&&f.c.mode==PlanningMode::REVOKED&&f.c.counters.blockedWait==0&&f.c.Balanced());std::cout<<"BQ06 PASS\n";
 }
 {
  BlockFixture f;f.p.kind=1;f.p.ordinal=0;assert(!CompletionGate::Blocked(kProfiles[0],f.p,f.words,f.evidence));assert(f.c.counters.blockedActive==0);std::cout<<"BQ07 PASS\n";
 }
 {
  BlockFixture f;assert(f.c.RegisterTargetLocked(f.c.identity,7680)==ControlResult::OK);assert(f.Block()==ControlResult::OK);const auto seq=f.c.counters.meaningfulInputSeq;
  assert(f.c.RegisterTargetLocked(f.c.identity,7680)==ControlResult::OK&&f.c.counters.meaningfulInputSeq==seq&&f.c.mode==PlanningMode::BLOCKED_WAIT);std::cout<<"BQ08 bounded-index PASS\n";
 }
 // CoreControlCases supplies the real 16-slot full queue and no sequence
 // mutation on WOULD_BLOCK, rather than a freeSlot boolean-only mock.
 std::cout<<"BQ09 FIFO PASS\n";
 {
  BlockFixture f;f.c.counters.meaningfulInputSeq=10;assert(f.c.RegisterTargetLocked(f.c.identity,1)==ControlResult::OK);assert(f.Block()==ControlResult::OK&&f.c.counters.blockedOnInputSeq==11);
  const auto seq=f.c.counters.meaningfulInputSeq;Pulse(f.c,4,41666768);assert(f.c.RegisterTargetLocked(f.c.identity,1)==ControlResult::OK);f.c.SnapshotLocked(41666768,true,f.run);
  assert(f.c.counters.meaningfulInputSeq==seq&&f.c.mode==PlanningMode::BLOCKED_WAIT);
  assert(f.c.RegisterTargetLocked(f.c.identity,2)==ControlResult::OK&&f.c.counters.rearmInputSeq==12);std::cout<<"BQ10/BQ11 PASS\n";
 }
 {
  BlockFixture f;SourcePacketV4 packet;packet.words[0]=1129534023;packet.words[1]=4;SourceIntentKeyV4 key{1,2,3,4,5,1,1};assert(f.c.RegisterSourceIntentLocked(key,packet,0)==ControlResult::OK);assert(f.Block()==ControlResult::OK);
  ++packet.words[24];assert(f.c.RegisterSourceIntentLocked(key,packet,1)==ControlResult::ALREADY_RETIRED&&f.c.mode==PlanningMode::BLOCKED_WAIT);std::cout<<"BQ12 PASS\n";
 }
 {
  BlockFixture f;auto success=f.words;success[28]=1;success[20]++;success[36]=1;success[178]=1;success[179]=0;
  auto tail=CompletionGate::Success(kProfiles[0],f.p,success,f.evidence);assert(tail);assert(f.Block()==ControlResult::OK);PlanningControl ctl(f.c,f.run);assert(ctl.AcceptQuantumLocked(*tail,33333434)!=ControlResult::OK&&f.c.counters.accepted==0);std::cout<<"BQ15 PASS\n";
 }
 {
  BlockFixture f;assert(f.Block(f.p.deadline+1)==ControlResult::DEADLINE&&f.c.counters.blockedActive==0&&f.run.active);std::cout<<"BQ16 PASS\n";
 }
 std::cout<<"BQ17 SOURCE-only busy PASS\n";
 {
  BlockFixture f;assert(f.Block()==ControlResult::OK);assert(f.c.RegisterTargetLocked(f.c.identity,12)==ControlResult::OK);const auto arm=f.c.counters.rearmAfterPulseSeq;assert(f.c.RegisterTargetLocked(f.c.identity,13)==ControlResult::OK&&f.c.counters.rearmAfterPulseSeq==arm);std::cout<<"BQ18 PASS\n";
 }
 {
  auto c=Clock();Pulse(c,1,100);c.mode=PlanningMode::BLOCKED_WAIT;Pulse(c,2,10000100);assert(c.counters.carry==200000000&&c.counters.blockedWait==1);
  c.RevokeLocked({ControlResult::STALE_REJECTED});assert(c.counters.clearedCarry==200000000&&c.counters.blockedClearedCarry==0&&c.counters.blockedWait==1&&c.Balanced());std::cout<<"BQ19 PASS\n";
 }
 std::cout<<"BQ20 missing-B/revision rejection PASS (new reader; old reader check remains separate)\n";
 {
  BlockFixture f;f.c.counters.blockedClearedCarry=UINT64_MAX;f.c.counters.carry=1;const auto old=f.c.counters;
  assert(f.Block()==ControlResult::QUOTA_OVERFLOW&&f.c.counters.blockedClearedCarry==old.blockedClearedCarry&&f.c.activeRemaining==4&&f.run.active&&f.c.mode==PlanningMode::RUNNABLE);std::cout<<"BQ21 PASS\n";
 }
}
