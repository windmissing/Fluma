#include "m3a/planning_control.h"
#include "m3a/quota_reader_v3.h"
#include "m3a/source_queue_v4.h"
#include <cassert>
#include <iostream>
using namespace grs::m3a;
static uint64_t Pair(const QuotaSnapshotV3& w,size_t i){return uint64_t(w[i])|(uint64_t(w[i+1])<<32);}
static void Put(Words256& w,size_t i,uint64_t n){w[i]=uint32_t(n);w[i+1]=uint32_t(n>>32);}
static ProgressClockV3 Clock(){
 ProgressClockV3 c;c.identity={1,2,3,4,5,6,7,8,9};c.counters.session=5;c.open=c.ready=c.uiFresh=c.gravityFresh=true;return c;
}
static void Pulse(ProgressClockV3& c,uint64_t seq,uint64_t now){assert(c.PulseLocked({c.identity,seq,now},now)==ControlResult::OK);assert(c.Balanced());}
static RootPermitV4 Permit(ProgressClockV3& c,RunControl& run,uint64_t now){
 RootPermitV4 p;p.identity=c.identity;p.plan=c.grant.id;p.nonce=run.nonce=41;p.control=51;p.frame=61;p.geometry=71;p.kind=2;p.ordinal=1;p.deadline=now+ProgressClockV3::RootNs;p.eventCapacity=100;p.physicsCommit=8;
 c.jobLive=run.jobLive=true;return p;
}
static Words256 Tail(const RootPermitV4& p){
 Words256 w{};w[0]=1381192263;w[1]=4;w[2]=256;w[3]=3235823838u;Put(w,4,p.nonce);Put(w,6,p.identity.run);Put(w,8,p.identity.owner);Put(w,10,p.identity.context);Put(w,12,p.identity.surface);Put(w,14,p.sourceCommit);Put(w,16,p.physicsCommit);Put(w,18,p.sourceCommit);Put(w,20,p.physicsCommit);Put(w,22,p.control);Put(w,24,p.frame);w[26]=p.ordinal;w[27]=2;w[28]=5;w[32]=p.baseBank;w[33]=p.candidateBank;w[35]=p.publicationSlot;Put(w,38,p.geometry);Put(w,40,p.identity.config);
 const uint32_t versions[]={1,1,1,3,4,4,4,15};for(size_t i=0;i<8;++i)w[42+i]=versions[i];w[54]=100;w[89]=192;w[91]=64;w[97]=w[110]=w[111]=549;w[99]=0x7fff;w[179]=3;w[173]=1;w[174]=7;w[184]=512;w[188]=1;return w;
}
static NativeRootEvidence Evidence(const RootPermitV4& p,uint64_t now){
 NativeRootEvidence e;e.submittedPasses=e.barriers=549;e.fenceSignalledNs=now;return e;
}
int main(){
 auto c=Clock();RunControl run;PlanGrantV3 grant;
 Pulse(c,1,100);Pulse(c,2,33333433);Pulse(c,3,33333434);assert(c.pending==4&&c.counters.carry==80);
 assert(c.ClaimLocked(33333434,false,run,grant));auto p=Permit(c,run,33333434);auto words=Tail(p);auto evidence=Evidence(p,33333434);
 auto blocked=CompletionGate::Blocked(kProfiles[0],p,words,evidence);assert(blocked);PlanningControl control(c,run);assert(control.BlockPlanningControlLocked(*blocked,33333434)==ControlResult::OK);assert(c.Balanced());assert(c.counters.blockedActive==4&&c.counters.blockedClearedCarry==80&&!run.active&&c.open&&c.ready&&c.baseline);
 assert(control.BlockPlanningControlLocked(*blocked,33333434)==ControlResult::ALREADY_RETIRED);assert(!c.ClaimLocked(33333434,true,run,grant));
 Pulse(c,4,41666768);assert(c.counters.blockedWait==1);assert(c.RegisterTargetLocked(c.identity,100)==ControlResult::OK);auto arm=c.counters.rearmAfterPulseSeq;assert(c.RegisterTargetLocked(c.identity,101)==ControlResult::OK&&arm==c.counters.rearmAfterPulseSeq);
 Pulse(c,5,50000102);assert(c.mode==PlanningMode::RUNNABLE&&c.pending==0&&c.counters.blockedWait==2);assert(!c.ClaimLocked(50000102,false,run,grant));assert(c.ClaimLocked(50000102,true,run,grant));assert(!c.ClaimLocked(50000102,true,run,grant));assert(c.RetireSourcePlanLocked(grant.id,run)==ControlResult::OK);
 auto snapshot=c.SnapshotLocked(50000102,true,run);assert(snapshot[0]==860963661&&snapshot[1]==3&&snapshot[3]==0xc0de0003&&Pair(snapshot,44)==4&&Pair(snapshot,48)==2);for(size_t i=100;i<256;++i)assert(snapshot[i]==0);assert(InspectQuotaSnapshotV3(snapshot).result==ControlResult::OK);
 for(size_t i:{size_t(1),size_t(3),size_t(11),size_t(100),size_t(50),size_t(99)}){auto corrupt=snapshot;corrupt[i]^=1;assert(InspectQuotaSnapshotV3(corrupt).result!=ControlResult::OK);}
 for(auto at:{size_t(1),size_t(3),size_t(49),size_t(97),size_t(99),size_t(173),size_t(188),size_t(189)}){auto bad=words;bad[at]^=1;assert(!CompletionGate::Blocked(kProfiles[0],p,bad,evidence));}
 auto badEvidence=evidence;badEvidence.fenceSignalledNs=p.deadline+1;assert(!CompletionGate::Blocked(kProfiles[0],p,words,badEvidence));badEvidence=evidence;badEvidence.gl=0x502;assert(!CompletionGate::Blocked(kProfiles[0],p,words,badEvidence));
 const auto same=c.counters.meaningfulInputSeq;assert(c.RegisterTargetLocked(c.identity,101)==ControlResult::OK&&c.counters.meaningfulInputSeq==same);
 auto wrong=c.identity;++wrong.context;assert(c.RegisterTargetLocked(wrong,102)==ControlResult::STALE_REJECTED);
 SourcePacketV4 packet;packet.words[0]=1129534023;packet.words[1]=4;
 SourceIntentKeyV4 key{c.identity.run,c.identity.owner,c.identity.context,c.identity.inputEpoch,c.identity.pageSession,1,1};
 assert(c.RegisterSourceIntentLocked(key,packet,16)==ControlResult::WOULD_BLOCK);assert(c.counters.meaningfulInputSeq==same);assert(c.RegisterSourceIntentLocked(key,packet,0)==ControlResult::OK);assert(c.RegisterSourceIntentLocked(key,packet,1)==ControlResult::ALREADY_RETIRED);
 assert(c.PulseLocked({c.identity,6,50000102+33333334},50000102+33333334)==ControlResult::CLOCK_GAP&&c.mode==PlanningMode::REVOKED&&c.Balanced());
 auto overflow=Clock();overflow.counters.pulses=UINT64_MAX;const auto old=overflow.counters;assert(overflow.PulseLocked({overflow.identity,1,10},10)==ControlResult::QUOTA_OVERFLOW&&overflow.counters.pulses==old.pulses&&!overflow.hasPulse);
 {
  auto queued=Clock();SourceQueueV4 fifo;RunControl producerRun;PlanGrantV3 sourceGrant;SourcePacketV4 out;
  packet.words[4]=1;packet.words[6]=1;packet.words[8]=7;packet.words[28]=8;
  Pulse(queued,1,100);queued.mode=PlanningMode::BLOCKED_WAIT;
  SourceIntentKeyV4 intent{1,2,3,4,5,1,1};
  assert(fifo.EnqueueLocked(queued,queued.identity,intent,packet)==ControlResult::OK&&fifo.SizeLocked()==1&&queued.mode==PlanningMode::REARM_PENDING);
  const auto seq=queued.counters.meaningfulInputSeq;
  assert(fifo.EnqueueLocked(queued,queued.identity,intent,packet)==ControlResult::ALREADY_RETIRED&&fifo.SizeLocked()==1&&queued.counters.meaningfulInputSeq==seq);
  assert(fifo.PeekForPlanLocked(queued,producerRun,sourceGrant,out)==ControlResult::WOULD_BLOCK);
  Pulse(queued,2,10000100);assert(queued.pending==0&&queued.mode==PlanningMode::RUNNABLE);
  assert(queued.ClaimLocked(10000100,true,producerRun,sourceGrant));assert(fifo.PeekForPlanLocked(queued,producerRun,sourceGrant,out)==ControlResult::OK);
  assert(fifo.PopLocked(queued,1,2)==ControlResult::STALE_REJECTED&&fifo.SizeLocked()==1);
  assert(fifo.PopLocked(queued,1,1)==ControlResult::OK&&fifo.SizeLocked()==0);
  for(uint64_t i=2;i<=17;++i){intent.userIntentSequence=i;packet.words[6]=uint32_t(i);assert(fifo.EnqueueLocked(queued,queued.identity,intent,packet)==ControlResult::OK);}
  const auto fullSeq=queued.counters.meaningfulInputSeq;intent.userIntentSequence=18;packet.words[6]=18;
  assert(fifo.EnqueueLocked(queued,queued.identity,intent,packet)==ControlResult::WOULD_BLOCK&&queued.counters.meaningfulInputSeq==fullSeq&&fifo.SizeLocked()==16);
  // This old grant froze boundary1. New tails may not be consumed by it.
  assert(fifo.PeekForPlanLocked(queued,producerRun,sourceGrant,out)==ControlResult::WOULD_BLOCK);
 }
 std::cout<<"T522 V3 BLOCKED/rearm/source-only/reader/late-tail/overflow control PASS\n";
}
