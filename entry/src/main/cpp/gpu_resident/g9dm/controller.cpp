#include "controller.h"
#include <chrono>
#include <string>
#include "wire.h"
#ifdef GRS_G9_BOOT_SOURCE
#include "source_trace.h"
#endif
namespace grs::g9dm {
namespace {uint64_t Now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}}
bool Controller::Check(void* user,const Permit& p,uint64_t now){auto& self=*static_cast<Controller*>(user);std::lock_guard<std::recursive_mutex> lock(self.mutex_);return self.RecheckLocked(p,std::max(now,Now()));}
bool Controller::RecheckLocked(const Permit& p,uint64_t now)const{
 return configured_&&rootLive_&&p==active_&&p.identity==identity_&&p.baseCommit==commit_&&now<=p.deadline&&clock_.Fresh(now)&&clock_.ready&&clock_.baseline&&clock_.mode==PlanningMode::RUNNABLE&&run_.active&&run_.plan==p.plan&&run_.nonce==p.nonce&&clock_.jobLive&&run_.jobLive;
}
ControlResult Controller::RevokeLocked(ControlResult why){
#ifdef GRS_G9_BOOT_SOURCE
 if (auto *trace = CurrentSourceTrace(); trace && !trace->fault && !trace->committed)
   trace->fault = uint32_t(why);
#endif
 const auto result=clock_.RevokeLocked({why});if(coldEpoch_!=UINT64_MAX)++coldEpoch_;rootLive_=false;run_.active=run_.sourcePhase=run_.jobLive=false;run_.plan=run_.nonce=0;run_.quantumOrdinal=0;stale_=lastGood_;return result;
}
Result Controller::ConfigureResearch(const Config& config,const ControlIdentity& id,const SeedFileIdentity& seed){
 {std::lock_guard<std::recursive_mutex> lock(mutex_);if(configured_||!config.Valid()||config.parameterSha!=ParameterHash(config))return Result::CALIBRATION_CONFIG_REQUIRED;}
 const auto result=transport_.Create(config,id,seed);if(result!=Result::OK)return result;
 // T564 (problem T560 §2): cold compute-capability export right after a successful create; the caller layers SCENE_ATTR separately.
 {const auto* profile=SelectProfile(config.k);grs::g9dm::ProfileRequirements need{profile?profile->stateBytes:0,profile?profile->workBytes:0,profile?profile->arenaBytes:0};StageCapabilityV2 caps{};if(transport_.QueryComputeCaps(need,&caps)==Result::OK)caps_.computeEligible=true;caps_.k=config.k;}
 std::lock_guard<std::recursive_mutex> lock(mutex_);identity_=id;config_=config;profile_=SelectProfile(config.k);clock_.identity=id;clock_.open=clock_.ready=clock_.uiFresh=clock_.gravityFresh=true;clock_.counters.session=id.pageSession;inputGate_.Configure(config,id);inputGate_.Pose(0,Now());
 layoutSha_=FromHex(kPhysicalLayoutSHA);shaderSha_=transport_.ShaderHash();configured_=true;return Result::OK;
}
Result Controller::OfferInput128(Input128 raw,uint64_t now,uint64_t sourceIntent){
 std::lock_guard<std::recursive_mutex> lock(mutex_);if(!configured_||!clock_.open||!clock_.ready)return Result::STALE;
#ifdef GRS_G9_BOOT_SOURCE
 if (raw[40]!=1||!sourceIntent||raw[24]!=0||nonce_)return Result::INVALID;
#endif
 const auto accepted=inputGate_.Offer(raw,now);if(accepted!=Result::OK)return accepted;
 if(raw[40]==0)return Result::OK;
 // A consumed identity is never a new FIFO/meaningful SOURCE intent. Native
 // compares only original command bytes and protocol identities, not physics.
 const auto& consumed=raw[40]==1?consumedSource_:consumedImpulse_;const bool hasConsumed=raw[40]==1?hasConsumedSource_:hasConsumedImpulse_;
 if(hasConsumed){const auto seq=Pair(raw.data(),41),old=Pair(consumed.data(),41);if(seq<old)return Result::STALE;if(seq==old)return SameCommand(consumed,raw)?Result::OK:Result::INVALID;}
 for(size_t i=0;i<size_;++i){const auto& old=queue_[(head_+i)%queue_.size()].input;if(old[40]==raw[40]&&Pair(old.data(),41)==Pair(raw.data(),41))return SameCommand(old,raw)?Result::OK:Result::INVALID;}
 if(size_==queue_.size())return Result::WOULD_BLOCK;
 auto nextClock=clock_;const auto slot=(head_+size_)%queue_.size();SourceIntentKey key{identity_.run,identity_.owner,identity_.context,identity_.inputEpoch,identity_.pageSession,clock_.sourceControlGeneration,sourceIntent};
 if(raw[40]==1){InputPacket packet;packet.words=raw;const auto result=nextClock.RegisterSourceIntentLocked(key,packet,slot);if(result!=ControlResult::OK)return result==ControlResult::WOULD_BLOCK?Result::WOULD_BLOCK:Result::INVALID;}
 // IMPULSE enqueue deliberately does not call a meaningful-input registration.
 queue_[slot]={raw,key,sourceIntent};clock_=nextClock;++size_;return Result::OK;
}
Result Controller::OfferModelCommand(const Input128& body,uint64_t now,uint64_t sourceIntent){
 Input128 raw;{std::lock_guard<std::recursive_mutex> lock(mutex_);if(!configured_||!inputGate_.ModelEnvelope(body,clock_.target,now,raw))return Result::INVALID;}
 return OfferInput128(raw,now,sourceIntent);
}
ControlResult Controller::Pulse(const PagePulseV3& pulse,uint64_t now){std::lock_guard<std::recursive_mutex> lock(mutex_);const auto result=clock_.PulseLocked(pulse,now);if(result==ControlResult::CLOCK_GAP||result==ControlResult::CONTROL_CLOCK_REVERSED||result==ControlResult::QUOTA_OVERFLOW)RevokeLocked(result);return result;}
ControlResult Controller::Target(const ControlIdentity& id,int32_t angle,uint64_t now){std::lock_guard<std::recursive_mutex> lock(mutex_);const auto result=clock_.RegisterTargetLocked(id,angle);if(result==ControlResult::OK)inputGate_.Pose(angle,now);return result;}
Result Controller::BeginRoot(uint64_t now,uint64_t frame,uint64_t geometry){
 if(!PollSceneFence())return Result::GL_FAILED;
 Input128 frozen;Permit permit;
 {std::lock_guard<std::recursive_mutex> lock(mutex_);now=std::max(now,Now());if(!configured_||rootLive_||transport_.Busy()||lease_||sceneFence_||coldBuilding_||!run_.workspaceRetired)return Result::WOULD_BLOCK;
  bool source=size_&&queue_[head_].input[40]==1;
#ifdef GRS_G9_BOOT_SOURCE
  // Check before ClaimLocked can move pending quota into a grant.
  if (auto *trace = CurrentSourceTrace()) {
    trace->pendingAtClaim = clock_.pending;
    trace->queueBefore = size_;
  }
  if(nonce_||run_.active||clock_.pending||clock_.target!=0||!source||
     !queue_[head_].intent||!clock_.baseline||!clock_.Fresh(now))
    return Result::WOULD_BLOCK;
#endif
  if(!run_.active){PlanGrantV3 grant;if(!clock_.ClaimLocked(now,source,run_,grant))return Result::WOULD_BLOCK;}
  // A source enqueued after the plan boundary waits for its own next plan.
  source=run_.sourcePhase;
  const Input128* command=nullptr;if(source){if(!size_||queue_[head_].input[40]!=1)return Result::WOULD_BLOCK;command=&queue_[head_].input;}
  else if(size_&&queue_[head_].input[40]==2)command=&queue_[head_].input;
  if(nonce_==UINT64_MAX||generation_==UINT64_MAX||frame<frame_||!geometry||!CheckedAdd(now,250000000,permit.deadline))return Result::INVALID;
  permit.rootBeginNs=now;permit.identity=identity_;permit.nonce=++nonce_;permit.plan=run_.plan;permit.frame=frame;permit.control=frame;permit.geometry=geometry;permit.baseCommit=commit_;permit.quanta=quanta_;permit.kind=source?1:2;permit.ordinal=source?0:run_.quantumOrdinal+1;permit.baseBank=transport_.CurrentBank();permit.candidateBank=2;permit.arena=uint32_t(generation_&1);permit.arenaGeneration=generation_+1;
  if(inputGate_.Freeze(command,permit.nonce,now,frozen)!=Result::OK){RevokeLocked(ControlResult::STALE_REJECTED);return Result::STALE;}
  frozen[24]=uint32_t(clock_.grant.target);permit.parameterSha=config_.parameterSha;permit.layoutSha=layoutSha_;permit.shaderSha=shaderSha_;permit.inputSha=HashWords(frozen.data(),frozen.size());
  active_=permit;rootLive_=true;clock_.jobLive=run_.jobLive=true;clock_.activeRootKind=permit.kind;run_.workspaceRetired=false;run_.nonce=permit.nonce;frame_=frame;
 }
 const auto result=transport_.Begin(permit,frozen,Check,this);if(result!=Result::OK){std::lock_guard<std::recursive_mutex> lock(mutex_);RevokeLocked(result==Result::DEADLINE?ControlResult::DEADLINE:ControlResult::STALE_REJECTED);}return result;
}
Result Controller::PumpBounded(uint64_t now){
 const auto result=transport_.Pump();if(result==Result::IN_PROGRESS)return result;
 if(result!=Result::OK){std::lock_guard<std::recursive_mutex> lock(mutex_);RevokeLocked(result==Result::DEADLINE?ControlResult::DEADLINE:ControlResult::STALE_REJECTED);return result;}
 const auto accepted=transport_.AcceptedProof();const auto blocked=transport_.BlockedProof();const auto tail=transport_.Tail();std::lock_guard<std::recursive_mutex> lock(mutex_);now=std::max(now,Now());
 if(!RecheckLocked(active_,now)){RevokeLocked(now>active_.deadline?ControlResult::DEADLINE:ControlResult::STALE_REJECTED);return now>active_.deadline?Result::DEADLINE:Result::STALE;}
 if(tail[6]==4){auto record=blocked;auto nextClock=clock_;auto nextRun=run_;PlanningControl nextPlanning(nextClock,nextRun);if(!record||nextPlanning.BlockPlanningControlLocked(*record,now)!=ControlResult::OK){RevokeLocked(ControlResult::INVALID_ARGUMENT);return Result::INVALID;}if(!transport_.RetireRejected(active_)){RevokeLocked(ControlResult::STALE_REJECTED);return Result::GL_FAILED;}clock_=nextClock;run_=nextRun;lastTail_=tail;rootLive_=false;run_.workspaceRetired=true;stale_=lastGood_;return Result::OK;}
 auto record=accepted;if(!record){RevokeLocked(ControlResult::INVALID_ARGUMENT);return Result::INVALID;}
#ifdef GRS_G9_BOOT_SOURCE
 if (auto *trace = CurrentSourceTrace()) {
   trace->typed = true;
   trace->expected = record->ExpectedHash();
   trace->activeRemaining = clock_.activeRemaining;
 }
 if(active_.kind!=1||clock_.activeRemaining||!size_||!queue_[head_].intent||
    clock_.intents[head_].sequence!=queue_[head_].intent||
    clock_.intents[head_].generation!=queue_[head_].key.sourceControlGeneration||
    !transport_.MatchesSourceCommand(queue_[head_].input)){
   RevokeLocked(ControlResult::STALE_REJECTED);return Result::STALE;
 }
#endif
 // Preflight all planning changes on copies; commit rechecks the same live flight under this recursive owner lock.
 auto nextClock=clock_;auto nextRun=run_;PlanningControl nextPlanning(nextClock,nextRun);
 if(active_.kind==2){if(nextPlanning.AcceptQuantumLocked(*record,now)!=ControlResult::OK){RevokeLocked(ControlResult::STALE_REJECTED);return Result::STALE;}}
 else{nextClock.jobLive=nextRun.jobLive=false;nextClock.activeRootKind=0;nextRun.sourcePhase=false;}
 if(generation_==UINT64_MAX){RevokeLocked(ControlResult::QUOTA_OVERFLOW);return Result::INVALID;}
 if(active_.kind==1&&nextClock.activeRemaining==0){
  // The accepted mapped flight has completed. Retire the proposed run only;
  // publication still requires CommitAccepted's live owner/permit check.
  nextRun.workspaceRetired=true;
  const auto retired=nextClock.RetireSourcePlanLocked(active_.plan,nextRun);
  if(retired!=ControlResult::OK){RevokeLocked(retired);return Result::INVALID;}
 }
#ifdef GRS_G9_BOOT_SOURCE
 if (auto *trace = CurrentSourceTrace()) {
   trace->preflight = true;
   trace->planRetired = !nextRun.active && !nextClock.grant.id && nextRun.workspaceRetired;
 }
#endif
 if(!transport_.CommitAccepted(*record)){RevokeLocked(ControlResult::STALE_REJECTED);return Result::STALE;}
 clock_=nextClock;run_=nextRun;
 if(size_&&((active_.kind==1&&queue_[head_].input[40]==1)||(active_.kind==2&&queue_[head_].input[40]==2))){if(queue_[head_].input[40]==1){consumedSource_=queue_[head_].input;hasConsumedSource_=true;}else{consumedImpulse_=queue_[head_].input;hasConsumedImpulse_=true;}clock_.intents[head_]={};queue_[head_]={};head_=(head_+1)%queue_.size();--size_;}
 commit_=Pair(tail.data(),30);quanta_=Pair(tail.data(),54);if(generation_==UINT64_MAX){RevokeLocked(ControlResult::QUOTA_OVERFLOW);return Result::INVALID;}if(tail[6]!=2)++generation_;lastTail_=tail;lastGood_=true;stale_=tail[6]==2;rootLive_=false;run_.workspaceRetired=true;
 if(run_.active&&active_.kind==2)run_.quantumOrdinal=active_.ordinal;
#ifdef GRS_G9_BOOT_SOURCE
 if (auto *trace = CurrentSourceTrace()) {
   trace->committed = true;
   trace->commit = commit_;
   trace->quanta = quanta_;
   trace->generation = generation_;
   trace->queueAfter = size_;
   trace->pendingAfter = clock_.pending;
 }
#endif

 return Result::OK;
}
QuotaSnapshotV3 Controller::Quota(uint64_t now)const{std::lock_guard<std::recursive_mutex> lock(mutex_);return clock_.SnapshotLocked(now,size_&&queue_[head_].input[40]==1,run_);}
Result Controller::Stop(){std::lock_guard<std::recursive_mutex> lock(mutex_);RevokeLocked(ControlResult::STALE_REJECTED);return Result::OK;}
bool Controller::Drain(){if(transport_.Busy()&&!transport_.RetireRejected(active_))return false;std::lock_guard<std::recursive_mutex> lock(mutex_);run_.workspaceRetired=true;return true;}
void Controller::Release(bool current){if(current&&sceneFence_)glDeleteSync(sceneFence_);sceneFence_=nullptr;transport_.Release(current);std::lock_guard<std::recursive_mutex> lock(mutex_);RevokeLocked(ControlResult::STALE_REJECTED);run_.workspaceRetired=true;configured_=lease_=false;}
Controller::SceneView Controller::AcquireScene(){if(!PollSceneFence())return {};std::lock_guard<std::recursive_mutex> lock(mutex_);if(!configured_||!lastGood_||rootLive_||transport_.Busy()||lease_||sceneFence_)return {};lease_=true;return {true,stale_,transport_.StateBuffer(transport_.CurrentBank()),generation_,identity_,config_.k};}
bool Controller::ReleaseScene(uint64_t generation,GLsync fence){std::lock_guard<std::recursive_mutex> lock(mutex_);if(!lease_||generation!=generation_||!fence||sceneFence_)return false;lease_=false;sceneFence_=fence;return true;}
bool Controller::PollSceneFence(){GLsync fence;{std::lock_guard<std::recursive_mutex> lock(mutex_);fence=sceneFence_;}if(!fence)return true;const auto result=glClientWaitSync(fence,0,0);if(result==GL_WAIT_FAILED)return false;if(result==GL_TIMEOUT_EXPIRED)return true;glDeleteSync(fence);std::lock_guard<std::recursive_mutex> lock(mutex_);if(sceneFence_==fence)sceneFence_=nullptr;return true;}
bool Controller::CanPresent(uint64_t generation,uint64_t now)const{std::lock_guard<std::recursive_mutex> lock(mutex_);return configured_&&lastGood_&&!stale_&&generation==generation_&&clock_.ready&&clock_.open&&clock_.Fresh(now)&&clock_.mode==PlanningMode::RUNNABLE&&now<=active_.deadline;}
Completion256 Controller::LastCompletion()const{std::lock_guard<std::recursive_mutex> lock(mutex_);return lastTail_;}
#ifdef GRS_G9_BOOT_SOURCE
GLenum Controller::PrepareSourceCache(){
 std::lock_guard<std::recursive_mutex> lock(mutex_);
 return configured_&&!rootLive_&&lastGood_ ? transport_.PrepareSourceCache() : GL_INVALID_OPERATION;
}
#endif
}
