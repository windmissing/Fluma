#include "controller.h"
#include <new>
#include <chrono>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
namespace grs::m3b {
namespace {uint64_t SteadyNow(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}U64 Limb(uint64_t value){return {uint32_t(value),uint32_t(value>>32)};}uint64_t Whole(U64 value){return uint64_t(value.lo)|(uint64_t(value.hi)<<32);}}
bool ControllerV5::Authorize(void* context,const RootPermitV5& p,uint64_t now){auto& self=*static_cast<ControllerV5*>(context);std::lock_guard<std::mutex> lock(self.mutex_);return self.owner_.AuthorizeLocked(p,std::max(now,SteadyNow()));}
ControlResult ControllerV5::Fail(ControlResult fault){
 std::lock_guard<std::mutex> lock(mutex_);
 if(clock_.fault!=ControlResult::OK)fault=clock_.fault;
 else if(fault==ControlResult::STALE_REJECTED&&clock_.hasPulse&&SteadyNow()>clock_.counters.receiveNs&&SteadyNow()-clock_.counters.receiveNs>ProgressClockV3::GapNs)fault=ControlResult::CLOCK_GAP;
 if(coldEpoch_!=UINT64_MAX)++coldEpoch_;return owner_.RevokeLocked({fault});
}
ControlResult ControllerV5::Configure(const Profile& p,const ControlIdentityV5& id,uint64_t geometry,const PhysicalConfigV5& config,
 const std::array<uint32_t,8>& shader,const std::array<uint32_t,8>& angle,const std::array<uint32_t,8>& layout){
 {std::lock_guard<std::mutex> lock(mutex_);if(configured_||run_.active||!run_.workspaceRetired||!config.Valid())return ControlResult::INVALID_ARGUMENT;clock_.identity=id;clock_.counters.session=id.pageSession;clock_.open=true;}
 const auto created=transport_.Create(p,id,geometry,config,shader,angle,layout);
 if(created!=TransportResultV5::OK)return Fail(ControlResult::UNSUPPORTED);
 std::lock_guard<std::mutex> lock(mutex_);
 if(clock_.identity!=id||!clock_.open)return ControlResult::STALE_REJECTED;
 const auto result=owner_.ConfigureLocked(p,geometry,shader,angle,layout);
 if(result!=ControlResult::OK)return result;
 profile_=&p;sceneIdentity_.run=Limb(id.run);sceneIdentity_.owner=Limb(id.owner);sceneIdentity_.context=Limb(id.context);sceneIdentity_.surface=Limb(id.surface);sceneIdentity_.geometry=Limb(geometry);sceneIdentity_.config=Limb(id.config);sceneIdentity_.shaderSha=shader;sceneIdentity_.angleSha=angle;sceneIdentity_.layoutSha=layout;
 liftParameters_=config.lift;const auto inputResult=liftInput_.Configure({id.inputEpoch,id.pageSession,id.owner,id.run,id.context},config.lift,config.lift.mode==LiftMode::SYNTHETIC_TEST);if(inputResult!=InputResult::OK)return ControlResult::INVALID_ARGUMENT;
 clock_.uiFresh=true;clock_.ready=clock_.gravityFresh=config.lift.mode==LiftMode::OFF;configured_=true;return ControlResult::OK;
}
ControlResult ControllerV5::Pulse(const PagePulseV3& pulse,uint64_t now){std::lock_guard<std::mutex> lock(mutex_);const auto r=clock_.PulseLocked(pulse,now);if(r==ControlResult::CLOCK_GAP||r==ControlResult::CONTROL_CLOCK_REVERSED||r==ControlResult::QUOTA_OVERFLOW){if(coldEpoch_!=UINT64_MAX)++coldEpoch_;owner_.RevokeLocked({r});}return r;}
ControlResult ControllerV5::Target(const ControlIdentityV5& id,int32_t target){std::lock_guard<std::mutex> lock(mutex_);return clock_.RegisterTargetLocked(id,target);}
ControlResult ControllerV5::Mode(const ControlIdentityV5& id,GravityMode mode){std::lock_guard<std::mutex> lock(mutex_);return clock_.RegisterModeLocked(id,mode);}
ControlResult ControllerV5::Offer(const ControlIdentityV5& id,const SourceIntentKeyV5& key,const SourcePacketV5& packet){std::lock_guard<std::mutex> lock(mutex_);return queue_.EnqueueLocked(clock_,id,key,packet);}
ControlResult ControllerV5::Advance(uint64_t now,uint64_t frame,uint64_t control){
 if(!PollSceneReleases())return Fail(ControlResult::STALE_REJECTED);
 LiftInput64 frozen{};SourcePacketV5 packet;bool source=false;int32_t target=0;uint32_t prefix=0,dedup=0;
 {
  std::lock_guard<std::mutex> lock(mutex_);
  now=std::max(now,SteadyNow());
  if(!configured_||coldBuilding_||(coldTrace_.builder>=0&&(!transport_.FullBranchMode()||(!coldPrepared_&&!run_.active)))||transport_.Busy()||owner_.HasRootLocked())return ControlResult::WOULD_BLOCK;
  if(!frame||!control||frame<frame_||control<control_)return ControlResult::SEQUENCE_REJECTED;
  if(!run_.active){PlanGrantV3 grant;if(!clock_.ClaimLocked(now,queue_.SizeLocked()!=0,run_,grant))return ControlResult::WOULD_BLOCK;transport_.SetCaptureGrant(grant,run_.sourcePhase);}
  const LiftInput64* input=nullptr;if(liftInput_.Freeze(clock_.grant.id,now,input)!=InputResult::OK){clock_.gravityFresh=false;owner_.RevokeLocked({ControlResult::STALE_REJECTED});return ControlResult::STALE_REJECTED;}frozen=*input;
  source=run_.sourcePhase;
  if(source){const auto peek=queue_.PeekForPlanLocked(clock_,run_,clock_.grant,packet);if(peek!=ControlResult::OK)return peek;sourceIntent_=queue_.FrontIntentLocked(clock_);}
  uint32_t publication=owner_.lastGood?1-owner_.publicationSlot:0;if(!leases_.WritableLocked(publication))return ControlResult::WOULD_BLOCK;
  const uint32_t start=source?0:frame==owner_.eventFrame?owner_.eventEnd:0;
  const auto begun=owner_.BeginRootLocked(now,source?1:2,source?0:run_.quantumOrdinal+1,frame,control,uint32_t(frame&1u),start,publication,transport_.PlannedUpload(source?1:2),active_);
  if(begun!=ControlResult::OK)return begun;
  transport_.SetCaptureQuota(clock_.SnapshotLocked(now,queue_.SizeLocked()!=0,run_),now,clock_.sourceControlGeneration);
  target=clock_.grant.target;prefix=owner_.sourcePrefix;dedup=owner_.dedupPrefix;frame_=frame;control_=control;
 }
 transport_.SetLiftInput(frozen);
 const auto started=transport_.Begin(active_,target,source?&packet:nullptr,prefix,dedup,Authorize,this);
 if(started!=TransportResultV5::OK)return Fail(started==TransportResultV5::DEADLINE?ControlResult::DEADLINE:ControlResult::STALE_REJECTED);
 return ControlResult::OK;
}
TransportResultV5 ControllerV5::Pump(uint64_t now){
 const auto pumped=transport_.Pump();
 if(pumped==TransportResultV5::IN_PROGRESS)return pumped;
 if(pumped!=TransportResultV5::OK){Fail(pumped==TransportResultV5::DEADLINE?ControlResult::DEADLINE:ControlResult::STALE_REJECTED);return pumped;}
 const auto tail=transport_.Tail();const auto evidence=transport_.Evidence();now=std::max(now,SteadyNow());
 std::lock_guard<std::mutex> lock(mutex_);
 now=std::max(now,SteadyNow());
 if(now>active_.deadline){owner_.RevokeLocked({ControlResult::DEADLINE});return TransportResultV5::DEADLINE;}
 if(!owner_.AuthorizeLocked(active_,now)){owner_.RevokeLocked({ControlResult::STALE_REJECTED});return TransportResultV5::STALE;}
 ControlResult result=ControlResult::INVALID_ARGUMENT;
 if(tail[28]==5){auto blocked=CompletionGate::Blocked(*profile_,active_,tail,evidence);if(blocked)result=owner_.BlockPlanningLocked(*blocked,now);}
 else if(tail[28]==1){auto success=CompletionGate::Success(*profile_,active_,tail,evidence);if(success)result=active_.kind==1?owner_.AcceptSourceLocked(*success,now):owner_.AcceptQuantumLocked(*success,now);}
 else{auto metadata=CompletionGate::Metadata(*profile_,active_,tail,evidence);if(metadata)result=owner_.AcceptSourceLocked(*metadata,now);}
 if(result!=ControlResult::OK){owner_.RevokeLocked({result});return TransportResultV5::STALE;}
 if(tail[28]==5){transport_.SealCaptureRoot(false,clock_.SnapshotLocked(now,queue_.SizeLocked()!=0,run_));if(!transport_.RetireRejected(active_))return TransportResultV5::GL_FAILED;if(transport_.FullTraceNeedsFreeze())run_.workspaceRetired=false;return TransportResultV5::OK;}
 if(!transport_.Commit(active_,tail)){owner_.RevokeLocked({ControlResult::INVALID_ARGUMENT});return TransportResultV5::GL_FAILED;}
 transport_.SealCaptureRoot(true,clock_.SnapshotLocked(now,queue_.SizeLocked()!=0,run_));
 PublicationTokenV5 token;token.identity=active_.identity;token.generation=owner_.publicationGeneration;token.source=owner_.sourceCommit;token.physics=owner_.physicsCommit;token.slot=owner_.publicationSlot;token.bank=owner_.currentBank;
 if(!leases_.PublishLocked(token)){owner_.RevokeLocked({ControlResult::INVALID_ARGUMENT});return TransportResultV5::STALE;}
 sceneHeader_[1]=5;sceneHeader_[27]=3;sceneHeader_[31]=31;sceneHeader_[32]=0;sceneHeader_[122]=1;sceneHeader_[124]=sceneHeader_[125]=2;
 sceneHeader_[41]=tail[88];sceneHeader_[42]=tail[90];sceneHeader_[43]=tail[89];sceneHeader_[44]=tail[91];sceneHeader_[45]=tail[107];sceneHeader_[72]=tail[36];sceneHeader_[73]=tail[37];
 if(active_.kind==1&&queue_.PopLocked(clock_,sourceIntent_.generation,sourceIntent_.sequence)!=ControlResult::OK){owner_.RevokeLocked({ControlResult::INVALID_ARGUMENT});return TransportResultV5::STALE;}
 if(!run_.active&&transport_.FullTraceNeedsFreeze())run_.workspaceRetired=false;
 return TransportResultV5::OK;
}
QuotaSnapshotV3 ControllerV5::Quota(uint64_t now)const{std::lock_guard<std::mutex> lock(mutex_);return clock_.SnapshotLocked(now,queue_.SizeLocked()!=0,run_);}
ControlResult ControllerV5::Stop(){return Fail(ControlResult::STALE_REJECTED);}
bool ControllerV5::DrainRevoked(){
 if(transport_.Busy()&&!transport_.RetireRejected(active_))return false;
 std::lock_guard<std::mutex> lock(mutex_);run_.workspaceRetired=true;return true;
}
void ControllerV5::Release(bool current){
 if(current)for(auto fence:releaseFences_)if(fence)glDeleteSync(fence);releaseFences_={};
 transport_.Release(current);std::lock_guard<std::mutex> lock(mutex_);owner_.RevokeLocked({ControlResult::STALE_REJECTED});run_.workspaceRetired=true;leases_.ContextLostLocked();configured_=false;sceneHeader_={};
}
Words256 ControllerV5::SceneHeaderV5()const{std::lock_guard<std::mutex> lock(mutex_);return sceneHeader_;}
ControllerV5::View ControllerV5::PublicationView()const{std::lock_guard<std::mutex> lock(mutex_);return {owner_.lastGood&&!owner_.stale&&!clock_.lastGoodStale&&clock_.open&&clock_.ready,owner_.publicationSlot,owner_.publicationGeneration};}
bool ControllerV5::CanPresent(uint64_t generation,uint64_t now)const{
 std::lock_guard<std::mutex> lock(mutex_);now=std::max(now,SteadyNow());
 return configured_&&owner_.lastGood&&!owner_.stale&&!clock_.lastGoodStale&&clock_.ready&&clock_.open&&clock_.Fresh(now)&&clock_.mode==PlanningMode::RUNNABLE&&active_.identity==clock_.identity&&generation==owner_.publicationGeneration&&now<=active_.deadline;
}
ControlCode ControllerV5::AcquireV5(uint32_t slot,U64 generation,PublicationLease& out,GLuint& buffer){
 if(!PollSceneReleases())return ControlCode::BUSY;
 std::lock_guard<std::mutex> lock(mutex_);
 if(!configured_||!owner_.lastGood||owner_.stale||clock_.lastGoodStale||!clock_.ready||!clock_.open)return ControlCode::STALE;
 PublicationTokenV5 token;const auto result=leases_.AcquireLocked(clock_.identity,slot,Whole(generation),token);if(result!=ControlResult::OK)return ControlCode::BUSY;
 drawTokens_[slot]=token;out={};out.identity=sceneIdentity_;out.generation=generation;out.source=Limb(token.source);out.physics=Limb(token.physics);out.control=Limb(active_.control);out.frame=Limb(active_.frame);out.nonce=Limb(active_.nonce);out.angle=sceneHeader_[45];out.bank=token.bank;out.available=out.leased=true;out.run=sceneIdentity_.run;out.context=sceneIdentity_.context;out.surface=sceneIdentity_.surface;buffer=transport_.Publication(slot);return ControlCode::OK;
}
ControlCode ControllerV5::ReleaseV5(uint32_t slot,U64 generation,GLsync fence){
 if(slot>=2||!fence)return ControlCode::INVALID_INPUT;
 std::lock_guard<std::mutex> lock(mutex_);
 if(releaseFences_[slot]||drawTokens_[slot].generation!=Whole(generation)||leases_.ReleaseLocked(drawTokens_[slot])!=ControlResult::OK)return ControlCode::STALE;
 releaseFences_[slot]=fence;return ControlCode::OK;
}
bool ControllerV5::PollSceneReleases(){
 for(uint32_t i=0;i<2;++i){
  GLsync fence;PublicationTokenV5 token;{std::lock_guard<std::mutex> lock(mutex_);fence=releaseFences_[i];token=drawTokens_[i];}
  if(!fence)continue;const auto result=glClientWaitSync(fence,0,0);
  if(result==GL_WAIT_FAILED)return false;if(result==GL_TIMEOUT_EXPIRED)continue;
  glDeleteSync(fence);
  std::lock_guard<std::mutex> lock(mutex_);if(releaseFences_[i]!=fence||leases_.RetireReleaseFenceLocked(token)!=ControlResult::OK)return false;releaseFences_[i]=nullptr;drawTokens_[i]={};
 }return true;
}
cold503::Scope ControllerV5::ColdScopeLocked()const{
 cold503::Scope scope;
#ifdef _WIN32
 scope.pid=uint64_t(_getpid());
#else
 scope.pid=uint64_t(getpid());
#endif
 scope.control.epoch=coldEpoch_;scope.control.run=clock_.identity.run;scope.control.context=clock_.identity.context;scope.control.surface=clock_.identity.surface;scope.control.session=clock_.identity.pageSession;scope.control.reset=clock_.identity.inputEpoch;scope.control.permission=clock_.identity.permission;
 std::strncpy(scope.route.data(),"m3b",scope.route.size()-1);std::strncpy(scope.source.data(),kDesignSha,scope.source.size()-1);return scope;
}
std::string ControllerV5::ColdBegin(uint64_t replace,uint64_t now){
 coldtrace::TraceManager::Receipt receipt;
 {std::lock_guard<std::mutex> lock(mutex_);receipt.scope=ColdScopeLocked();cold::Layout layout;const bool full=transport_.FullBranchMode();
  if(!configured_||(!full&&(run_.active||run_.jobLive||transport_.Busy()))||!cold::Layout::Make(*profile_,transport_.StorageAlignment(),layout,full))receipt.result="WOULD_BLOCK";
  else{
   receipt.result=coldTrace_.Begin(receipt.scope,receipt.scope,now,replace,layout.words,receipt.request,receipt.id,layout.words-256,full?1u:0u);
   if(!std::strcmp(receipt.result,"REQUESTED")){
    if(full&&!transport_.ReserveFullCredit()){coldTrace_.Close(coldTrace_.builder,"FULL_UNSUPPORTED_RESOURCE");coldTrace_.Start();receipt.result="FULL_UNSUPPORTED_RESOURCE";}
    else{coldPrepared_=false;coldBuilding_=!full;}
   }
  }
 }
 std::array<char,16384> data{};cold::Writer w(data.data(),data.size());coldtrace::ReceiptJson(w,receipt);return std::string(w.Data(),w.Size());
}
std::string ControllerV5::ColdCapabilities()const{
 std::array<char,2048> bytes{};cold::Writer writer(bytes.data(),bytes.size());
 {std::lock_guard<std::mutex> lock(mutex_);if(!configured_||!cold::Capabilities(writer,*profile_,transport_.StorageAlignment(),transport_.FullBranchMode(),transport_.CaptureFixedMetadataBytes()))return "{\"state\":\"UNSUPPORTED_M3_COLD\"}";}
 return std::string(writer.Data(),writer.Size());
}
std::string ControllerV5::SourceStats()const{
 SourceQueueV5::Stats stats;{std::lock_guard<std::mutex> lock(mutex_);stats=queue_.StatsLocked();}
 std::array<char,512> bytes{};cold::Writer w(bytes.data(),bytes.size());
 w.Raw("{\"revision\":\"GRS_M3_SOURCE_PRODUCER_V5\",\"offered\":");w.U64(stats.offered);w.Raw(",\"enqueued\":");w.U64(stats.enqueued);w.Raw(",\"backpressured\":");w.U64(stats.backpressured);w.Raw(",\"replayed\":");w.U64(stats.replayed);w.Raw(",\"rejected\":");w.U64(stats.rejected);w.Raw("}");return std::string(w.Data(),w.Size());
}
std::string ControllerV5::ColdState(uint64_t request,uint64_t now){
 coldtrace::TraceManager::Receipt receipt;{std::lock_guard<std::mutex> lock(mutex_);receipt=coldTrace_.StateOf(request,now,ColdScopeLocked());}
 std::array<char,16384> data{};cold::Writer w(data.data(),data.size());coldtrace::ReceiptJson(w,receipt);return std::string(w.Data(),w.Size());
}
std::string ControllerV5::ColdPage(uint64_t trace,uint32_t page,uint64_t now){
 coldtrace::TraceManager::Read read;const char* result;
 {std::lock_guard<std::mutex> lock(mutex_);result=coldTrace_.Acquire(trace,page,now,ColdScopeLocked(),read);}
 if(std::strcmp(result,"OK"))return std::string("{\"state\":\"")+result+"\"}";
 struct ReleaseRead {ControllerV5* self;coldtrace::TraceManager::Read read;uint64_t now;bool active=true;~ReleaseRead(){if(active){std::lock_guard<std::mutex> lock(self->mutex_);self->coldTrace_.Release(read,now,self->ColdScopeLocked());}}} release{this,read,now};
 std::array<char,16384> data{};cold::Writer w(data.data(),data.size());coldtrace::TracePage(w,read,page);
 {std::lock_guard<std::mutex> lock(mutex_);result=coldTrace_.Release(read,now,ColdScopeLocked());release.active=false;}
 if(!w.Good()||std::strcmp(result,"TRACE_READY"))return "{\"state\":\"INCOMPLETE_SCOPE_CHANGED\"}";
 return std::string(w.Data(),w.Size());
}
std::string ControllerV5::ColdEnd(uint64_t trace,uint64_t now){
 std::lock_guard<std::mutex> lock(mutex_);const auto result=coldTrace_.End(trace,now,ColdScopeLocked());return std::string("{\"revision\":\"T527_V5_BOUNDED_COLD_V2\",\"state\":\"")+result+"\"}";
}
void ControllerV5::BuildCold(uint64_t now){
 int slot=-1;bool full=false,cancel=false,prepare=false;size_t allocate=0;std::unique_ptr<uint32_t[]> old;
 {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.Maintain(now,ColdScopeLocked());slot=coldTrace_.builder;if(slot<0)return;
  auto& record=coldTrace_.slots[slot];full=record.fullRows!=0;cancel=record.cancel;
  if(transport_.Busy()||run_.jobLive)return;
  if(!cancel&&full&&!coldPrepared_&&run_.active)return; // next complete plan boundary
  if(!record.started&&!cancel){if(coldTrace_.Start()<0)return;old=coldTrace_.TakeOld(slot);allocate=record.reserved;prepare=true;}
 }
 old.reset();
 if(prepare){
  {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.OldFreed(slot);}
  std::unique_ptr<uint32_t[]> block;if(allocate)block.reset(new(std::nothrow)uint32_t[allocate]);
  {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.Adopt(slot,std::move(block));if(!coldTrace_.slots[slot].block)cancel=true;}
  if(full&&!cancel){const bool armed=transport_.ArmFullCapture();std::lock_guard<std::mutex> lock(mutex_);if(armed)coldPrepared_=true;else cancel=true;}
 }
 if(cancel){
  transport_.RetireFullCapture();
  std::lock_guard<std::mutex> lock(mutex_);coldTrace_.Finish(slot,ColdScopeLocked(),now,"CAPTURE_CANCELLED_OR_ALLOCATION_FAILED",0,0);coldBuilding_=coldPrepared_=false;run_.workspaceRetired=true;return;
 }
 uint32_t* storage=nullptr;size_t words=0;uint64_t request=0,trace=0,created=0;QuotaSnapshotV3 quota;cold503::Scope scope;
 {std::lock_guard<std::mutex> lock(mutex_);
  if(full&&(!transport_.CaptureStarted()||run_.active))return;
  coldBuilding_=true;auto& record=coldTrace_.slots[slot];storage=record.block.get();words=record.capacity;request=record.request;trace=record.id;created=record.created;scope=record.scope;quota=clock_.SnapshotLocked(now,queue_.SizeLocked()!=0,run_);
 }
 size_t state=0,events=0;
 const bool copied=storage&&transport_.CaptureColdV5(storage,words,quota,state,events,request,trace,created,&scope);
 bool validated=false;if(copied){const auto* profile=SelectProfile(storage[8],storage[9]);cold::Layout layout;validated=profile&&cold::Layout::Make(*profile,4,layout,full)&&state+256==layout.words&&events==0&&cold::Validate(storage,words,layout);}
 if(full)transport_.RetireFullCapture();
 {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.Finish(slot,ColdScopeLocked(),now,copied?"OK":transport_.CaptureFailure(),state,events,transport_.FirstGlError(),transport_.FirstEglError()==EGL_SUCCESS?0:uint32_t(transport_.FirstEglError()),copied?"":"V5_OWNER_CAPTURE",0,validated);coldBuilding_=coldPrepared_=false;run_.workspaceRetired=true;}
}

ControllerV5::RootDiagnostic ControllerV5::DiagnosticOwnerThread()const{std::lock_guard<std::mutex> lock(mutex_);return {active_,transport_.Tail(),transport_.Evidence(),clock_.fault};}

InputResult ControllerV5::OfferLiftInput(const LiftInput64& raw,uint64_t receipt){
 std::lock_guard<std::mutex> lock(mutex_);if(!configured_||!clock_.open)return InputResult::STALE_INPUT;
 const auto result=liftInput_.Offer(raw,receipt);
 if(result!=InputResult::OK){clock_.gravityFresh=false;owner_.RevokeLocked({ControlResult::STALE_REJECTED});if(coldEpoch_!=UINT64_MAX)++coldEpoch_;return result;}
 if(!clock_.ready){clock_.ready=true;clock_.baseline=false;}clock_.gravityFresh=true;return result;
}
ControlResult ControllerV5::ResumeLiftSession(const ControlIdentityV5& id){
 std::lock_guard<std::mutex> lock(mutex_);const auto old=clock_.identity;
 if(!configured_||run_.active||run_.jobLive||!run_.workspaceRetired||owner_.HasRootLocked())return ControlResult::WOULD_BLOCK;
 if(id.run!=old.run||id.owner!=old.owner||id.context!=old.context||id.surface!=old.surface||id.window!=old.window||id.config!=old.config||id.permission!=old.permission||id.inputEpoch<=old.inputEpoch||id.pageSession<=old.pageSession)return ControlResult::STALE_REJECTED;
 if(clock_.sourceControlGeneration==UINT64_MAX)return ControlResult::QUOTA_OVERFLOW;
 const auto result=clock_.BeginSessionLocked({id});if(result!=ControlResult::OK)return result;
 ++clock_.sourceControlGeneration;queue_=SourceQueueV5{};
 if(liftInput_.Configure({id.inputEpoch,id.pageSession,id.owner,id.run,id.context},liftParameters_,liftParameters_.mode==LiftMode::SYNTHETIC_TEST)!=InputResult::OK)return ControlResult::INVALID_ARGUMENT;
 clock_.ready=clock_.gravityFresh=liftParameters_.mode==LiftMode::OFF;clock_.uiFresh=true;clock_.baseline=false;return ControlResult::OK;
}
GLuint ControllerV5::StateBufferV5(const PublicationLease& lease)const{
 std::lock_guard<std::mutex> lock(mutex_);if(!lease.leased||lease.bank!=owner_.currentBank||Whole(lease.generation)!=owner_.publicationGeneration)return 0;
 return transport_.StateBuffer(lease.bank);
}
}
