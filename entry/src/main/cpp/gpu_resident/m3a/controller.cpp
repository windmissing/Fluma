#include "controller.h"
#include <new>
#include <chrono>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
namespace grs::m3a {
namespace {uint64_t SteadyNow(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}U64 Limb(uint64_t value){return {uint32_t(value),uint32_t(value>>32)};}uint64_t Whole(U64 value){return uint64_t(value.lo)|(uint64_t(value.hi)<<32);}}
bool ControllerV4::Authorize(void* context,const RootPermitV4& p,uint64_t now){auto& self=*static_cast<ControllerV4*>(context);std::lock_guard<std::mutex> lock(self.mutex_);return self.owner_.AuthorizeLocked(p,std::max(now,SteadyNow()));}
ControlResult ControllerV4::Fail(ControlResult fault){
 std::lock_guard<std::mutex> lock(mutex_);
 if(clock_.fault!=ControlResult::OK)fault=clock_.fault;
 else if(fault==ControlResult::STALE_REJECTED&&clock_.hasPulse&&SteadyNow()>clock_.counters.receiveNs&&SteadyNow()-clock_.counters.receiveNs>ProgressClockV3::GapNs)fault=ControlResult::CLOCK_GAP;
 if(coldEpoch_!=UINT64_MAX)++coldEpoch_;return owner_.RevokeLocked({fault});
}
ControlResult ControllerV4::Configure(const Profile& p,const ControlIdentityV4& id,uint64_t geometry,const PhysicalConfigV4& config,
 const std::array<uint32_t,8>& shader,const std::array<uint32_t,8>& angle,const std::array<uint32_t,8>& layout){
 {std::lock_guard<std::mutex> lock(mutex_);if(configured_||run_.active||!run_.workspaceRetired||!config.Valid())return ControlResult::INVALID_ARGUMENT;clock_.identity=id;clock_.counters.session=id.pageSession;clock_.open=true;}
 const auto created=transport_.Create(p,id,geometry,config,shader,angle,layout);
 if(created!=TransportResultV4::OK)return Fail(ControlResult::UNSUPPORTED);
 std::lock_guard<std::mutex> lock(mutex_);
 if(clock_.identity!=id||!clock_.open)return ControlResult::STALE_REJECTED;
 const auto result=owner_.ConfigureLocked(p,geometry,shader,angle,layout);
 if(result!=ControlResult::OK)return result;
 profile_=&p;sceneIdentity_.run=Limb(id.run);sceneIdentity_.owner=Limb(id.owner);sceneIdentity_.context=Limb(id.context);sceneIdentity_.surface=Limb(id.surface);sceneIdentity_.geometry=Limb(geometry);sceneIdentity_.config=Limb(id.config);sceneIdentity_.shaderSha=shader;sceneIdentity_.angleSha=angle;sceneIdentity_.layoutSha=layout;
 clock_.ready=clock_.uiFresh=clock_.gravityFresh=true;configured_=true;return ControlResult::OK;
}
ControlResult ControllerV4::Pulse(const PagePulseV3& pulse,uint64_t now){std::lock_guard<std::mutex> lock(mutex_);const auto r=clock_.PulseLocked(pulse,now);if(r==ControlResult::CLOCK_GAP||r==ControlResult::CONTROL_CLOCK_REVERSED||r==ControlResult::QUOTA_OVERFLOW){if(coldEpoch_!=UINT64_MAX)++coldEpoch_;owner_.RevokeLocked({r});}return r;}
ControlResult ControllerV4::Target(const ControlIdentityV4& id,int32_t target){std::lock_guard<std::mutex> lock(mutex_);return clock_.RegisterTargetLocked(id,target);}
ControlResult ControllerV4::Mode(const ControlIdentityV4& id,GravityMode mode){std::lock_guard<std::mutex> lock(mutex_);return clock_.RegisterModeLocked(id,mode);}
ControlResult ControllerV4::Offer(const ControlIdentityV4& id,const SourceIntentKeyV4& key,const SourcePacketV4& packet){std::lock_guard<std::mutex> lock(mutex_);return queue_.EnqueueLocked(clock_,id,key,packet);}
ControlResult ControllerV4::Advance(uint64_t now,uint64_t frame,uint64_t control){
 if(!PollSceneReleases())return Fail(ControlResult::STALE_REJECTED);
 SourcePacketV4 packet;bool source=false;int32_t target=0;uint32_t prefix=0,dedup=0;
 {
  std::lock_guard<std::mutex> lock(mutex_);
  now=std::max(now,SteadyNow());
  if(!configured_||coldTrace_.builder>=0||transport_.Busy()||owner_.HasRootLocked())return ControlResult::WOULD_BLOCK;
  if(!frame||!control||frame<frame_||control<control_)return ControlResult::SEQUENCE_REJECTED;
  if(!run_.active){PlanGrantV3 grant;if(!clock_.ClaimLocked(now,queue_.SizeLocked()!=0,run_,grant))return ControlResult::WOULD_BLOCK;}
  source=run_.sourcePhase;
  if(source){const auto peek=queue_.PeekForPlanLocked(clock_,run_,clock_.grant,packet);if(peek!=ControlResult::OK)return peek;sourceIntent_=queue_.FrontIntentLocked(clock_);}
  uint32_t publication=owner_.lastGood?1-owner_.publicationSlot:0;if(!leases_.WritableLocked(publication))return ControlResult::WOULD_BLOCK;
  const uint32_t start=source?0:frame==owner_.eventFrame?owner_.eventEnd:0;
  const auto begun=owner_.BeginRootLocked(now,source?1:2,source?0:run_.quantumOrdinal+1,frame,control,uint32_t(frame&1u),start,publication,transport_.PlannedUpload(source?1:2),active_);
  if(begun!=ControlResult::OK)return begun;
  target=clock_.grant.target;prefix=owner_.sourcePrefix;dedup=owner_.dedupPrefix;frame_=frame;control_=control;
 }
 const auto started=transport_.Begin(active_,target,source?&packet:nullptr,prefix,dedup,Authorize,this);
 if(started!=TransportResultV4::OK)return Fail(started==TransportResultV4::DEADLINE?ControlResult::DEADLINE:ControlResult::STALE_REJECTED);
 return ControlResult::OK;
}
TransportResultV4 ControllerV4::Pump(uint64_t now){
 const auto pumped=transport_.Pump();
 if(pumped==TransportResultV4::IN_PROGRESS)return pumped;
 if(pumped!=TransportResultV4::OK){Fail(pumped==TransportResultV4::DEADLINE?ControlResult::DEADLINE:ControlResult::STALE_REJECTED);return pumped;}
 const auto tail=transport_.Tail();const auto evidence=transport_.Evidence();now=std::max(now,SteadyNow());
 std::lock_guard<std::mutex> lock(mutex_);
 now=std::max(now,SteadyNow());
 if(now>active_.deadline){owner_.RevokeLocked({ControlResult::DEADLINE});return TransportResultV4::DEADLINE;}
 if(!owner_.AuthorizeLocked(active_,now)){owner_.RevokeLocked({ControlResult::STALE_REJECTED});return TransportResultV4::STALE;}
 ControlResult result=ControlResult::INVALID_ARGUMENT;
 if(tail[28]==5){auto blocked=CompletionGate::Blocked(*profile_,active_,tail,evidence);if(blocked)result=owner_.BlockPlanningLocked(*blocked,now);}
 else if(tail[28]==1){auto success=CompletionGate::Success(*profile_,active_,tail,evidence);if(success)result=active_.kind==1?owner_.AcceptSourceLocked(*success,now):owner_.AcceptQuantumLocked(*success,now);}
 else{auto metadata=CompletionGate::Metadata(*profile_,active_,tail,evidence);if(metadata)result=owner_.AcceptSourceLocked(*metadata,now);}
 if(result!=ControlResult::OK){owner_.RevokeLocked({result});return TransportResultV4::STALE;}
 if(tail[28]==5){if(!transport_.RetireRejected(active_))return TransportResultV4::GL_FAILED;if(transport_.FullTraceNeedsFreeze())run_.workspaceRetired=false;return TransportResultV4::OK;}
 if(!transport_.Commit(active_,tail)){owner_.RevokeLocked({ControlResult::INVALID_ARGUMENT});return TransportResultV4::GL_FAILED;}
 PublicationTokenV4 token;token.identity=active_.identity;token.generation=owner_.publicationGeneration;token.source=owner_.sourceCommit;token.physics=owner_.physicsCommit;token.slot=owner_.publicationSlot;token.bank=owner_.currentBank;
 if(!leases_.PublishLocked(token)){owner_.RevokeLocked({ControlResult::INVALID_ARGUMENT});return TransportResultV4::STALE;}
 sceneHeader_[1]=4;sceneHeader_[27]=3;sceneHeader_[31]=15;sceneHeader_[32]=0;sceneHeader_[122]=1;sceneHeader_[124]=sceneHeader_[125]=2;
 sceneHeader_[41]=tail[88];sceneHeader_[42]=tail[90];sceneHeader_[43]=tail[89];sceneHeader_[44]=tail[91];sceneHeader_[45]=tail[107];sceneHeader_[72]=tail[36];sceneHeader_[73]=tail[37];
 if(active_.kind==1&&queue_.PopLocked(clock_,sourceIntent_.generation,sourceIntent_.sequence)!=ControlResult::OK){owner_.RevokeLocked({ControlResult::INVALID_ARGUMENT});return TransportResultV4::STALE;}
 if(!run_.active&&transport_.FullTraceNeedsFreeze())run_.workspaceRetired=false;
 return TransportResultV4::OK;
}
QuotaSnapshotV3 ControllerV4::Quota(uint64_t now)const{std::lock_guard<std::mutex> lock(mutex_);return clock_.SnapshotLocked(now,queue_.SizeLocked()!=0,run_);}
ControlResult ControllerV4::Stop(){return Fail(ControlResult::STALE_REJECTED);}
bool ControllerV4::DrainRevoked(){
 if(transport_.Busy()&&!transport_.RetireRejected(active_))return false;
 std::lock_guard<std::mutex> lock(mutex_);run_.workspaceRetired=true;return true;
}
void ControllerV4::Release(bool current){
 if(current)for(auto fence:releaseFences_)if(fence)glDeleteSync(fence);releaseFences_={};
 transport_.Release(current);std::lock_guard<std::mutex> lock(mutex_);owner_.RevokeLocked({ControlResult::STALE_REJECTED});run_.workspaceRetired=true;leases_.ContextLostLocked();configured_=false;sceneHeader_={};
}
Words256 ControllerV4::SceneHeaderV4()const{std::lock_guard<std::mutex> lock(mutex_);return sceneHeader_;}
ControllerV4::View ControllerV4::PublicationView()const{std::lock_guard<std::mutex> lock(mutex_);return {owner_.lastGood&&!owner_.stale&&!clock_.lastGoodStale&&clock_.open&&clock_.ready,owner_.publicationSlot,owner_.publicationGeneration};}
bool ControllerV4::CanPresent(uint64_t generation,uint64_t now)const{
 std::lock_guard<std::mutex> lock(mutex_);now=std::max(now,SteadyNow());
 return configured_&&owner_.lastGood&&!owner_.stale&&!clock_.lastGoodStale&&clock_.ready&&clock_.open&&clock_.Fresh(now)&&clock_.mode==PlanningMode::RUNNABLE&&active_.identity==clock_.identity&&generation==owner_.publicationGeneration&&now<=active_.deadline;
}
ControlCode ControllerV4::AcquireV4(uint32_t slot,U64 generation,PublicationLease& out,GLuint& buffer){
 if(!PollSceneReleases())return ControlCode::BUSY;
 std::lock_guard<std::mutex> lock(mutex_);
 if(!configured_||!owner_.lastGood||owner_.stale||clock_.lastGoodStale||!clock_.ready||!clock_.open)return ControlCode::STALE;
 PublicationTokenV4 token;const auto result=leases_.AcquireLocked(clock_.identity,slot,Whole(generation),token);if(result!=ControlResult::OK)return ControlCode::BUSY;
 drawTokens_[slot]=token;out={};out.identity=sceneIdentity_;out.generation=generation;out.source=Limb(token.source);out.physics=Limb(token.physics);out.control=Limb(active_.control);out.frame=Limb(active_.frame);out.nonce=Limb(active_.nonce);out.angle=sceneHeader_[45];out.bank=token.bank;out.available=out.leased=true;out.run=sceneIdentity_.run;out.context=sceneIdentity_.context;out.surface=sceneIdentity_.surface;buffer=transport_.Publication(slot);return ControlCode::OK;
}
ControlCode ControllerV4::ReleaseV4(uint32_t slot,U64 generation,GLsync fence){
 if(slot>=2||!fence)return ControlCode::INVALID_INPUT;
 std::lock_guard<std::mutex> lock(mutex_);
 if(releaseFences_[slot]||drawTokens_[slot].generation!=Whole(generation)||leases_.ReleaseLocked(drawTokens_[slot])!=ControlResult::OK)return ControlCode::STALE;
 releaseFences_[slot]=fence;return ControlCode::OK;
}
bool ControllerV4::PollSceneReleases(){
 for(uint32_t i=0;i<2;++i){
  GLsync fence;PublicationTokenV4 token;{std::lock_guard<std::mutex> lock(mutex_);fence=releaseFences_[i];token=drawTokens_[i];}
  if(!fence)continue;const auto result=glClientWaitSync(fence,0,0);
  if(result==GL_WAIT_FAILED)return false;if(result==GL_TIMEOUT_EXPIRED)continue;
  glDeleteSync(fence);
  std::lock_guard<std::mutex> lock(mutex_);if(releaseFences_[i]!=fence||leases_.RetireReleaseFenceLocked(token)!=ControlResult::OK)return false;releaseFences_[i]=nullptr;drawTokens_[i]={};
 }return true;
}
cold503::Scope ControllerV4::ColdScopeLocked()const{
 cold503::Scope scope;
#ifdef _WIN32
 scope.pid=uint64_t(_getpid());
#else
 scope.pid=uint64_t(getpid());
#endif
 scope.control.epoch=coldEpoch_;scope.control.run=clock_.identity.run;scope.control.context=clock_.identity.context;scope.control.surface=clock_.identity.surface;scope.control.session=clock_.identity.pageSession;scope.control.reset=clock_.identity.inputEpoch;scope.control.permission=clock_.identity.permission;
 std::strncpy(scope.route.data(),"m3a",scope.route.size()-1);std::strncpy(scope.source.data(),kDesignSha,scope.source.size()-1);return scope;
}
std::string ControllerV4::ColdBegin(uint64_t replace,uint64_t now){
 coldtrace::TraceManager::Receipt receipt;
 {std::lock_guard<std::mutex> lock(mutex_);const auto scope=ColdScopeLocked();receipt.scope=scope;
  cold::Layout layout;if(!configured_||run_.jobLive||((transport_.FullBranchMode()&&run_.active)||(!run_.workspaceRetired&&!transport_.FullTraceNeedsFreeze()))||(!owner_.lastGood&&!transport_.HasColdTail())||!clock_.open||!cold::Layout::Make(*profile_,transport_.StorageAlignment(),layout,transport_.FullBranchMode()))receipt.result="WOULD_BLOCK";
  else receipt.result=coldTrace_.Begin(scope,scope,now,replace,layout.words,receipt.request,receipt.id,layout.events-256,transport_.FullBranchMode()?profile_->scopeRows:0);
 }
 std::array<char,cold::kPageBytes> bytes{};cold::Writer w(bytes.data(),bytes.size());coldtrace::ReceiptJson(w,receipt);return std::string(w.Data(),w.Size());
}
std::string ControllerV4::ColdCapabilities()const{
 std::array<char,2048> bytes{};cold::Writer writer(bytes.data(),bytes.size());
 {std::lock_guard<std::mutex> lock(mutex_);if(!configured_||!cold::Capabilities(writer,*profile_,transport_.StorageAlignment(),transport_.FullBranchMode()))return "{\"state\":\"UNSUPPORTED_M3_COLD\"}";}
 return std::string(writer.Data(),writer.Size());
}
std::string ControllerV4::SourceStats()const{
 SourceQueueV4::Stats stats;{std::lock_guard<std::mutex> lock(mutex_);stats=queue_.StatsLocked();}
 std::array<char,512> bytes{};cold::Writer w(bytes.data(),bytes.size());
 w.Raw("{\"revision\":\"GRS_M3_SOURCE_PRODUCER_V4\",\"offered\":");w.U64(stats.offered);w.Raw(",\"enqueued\":");w.U64(stats.enqueued);w.Raw(",\"backpressured\":");w.U64(stats.backpressured);w.Raw(",\"replayed\":");w.U64(stats.replayed);w.Raw(",\"rejected\":");w.U64(stats.rejected);w.Raw("}");return std::string(w.Data(),w.Size());
}
std::string ControllerV4::ColdState(uint64_t request,uint64_t now){
 coldtrace::TraceManager::Receipt receipt;{std::lock_guard<std::mutex> lock(mutex_);receipt=coldTrace_.StateOf(request,now,ColdScopeLocked());}
 std::array<char,cold::kPageBytes> bytes{};cold::Writer w(bytes.data(),bytes.size());coldtrace::ReceiptJson(w,receipt);return std::string(w.Data(),w.Size());
}
std::string ControllerV4::ColdPage(uint64_t trace,uint32_t page,uint64_t now){
 coldtrace::TraceManager::Read read;const char* result;
 {std::lock_guard<std::mutex> lock(mutex_);result=coldTrace_.Acquire(trace,page,now,ColdScopeLocked(),read);}
 if(std::strcmp(result,"OK"))return std::string("{\"revision\":\"GRS_M3_BOUNDED_COLD_V1\",\"state\":\"")+result+"\"}";
 std::array<char,cold::kPageBytes> bytes{};cold::Writer w(bytes.data(),bytes.size());coldtrace::TracePage(w,read,page);
 {std::lock_guard<std::mutex> lock(mutex_);result=coldTrace_.Release(read,now,ColdScopeLocked());}
 if(!w.Good()||std::strcmp(result,"TRACE_READY"))return "{\"state\":\"INCOMPLETE\"}";
 return std::string(w.Data(),w.Size());
}
std::string ControllerV4::ColdEnd(uint64_t trace,uint64_t now){std::lock_guard<std::mutex> lock(mutex_);const auto result=coldTrace_.End(trace,now,ColdScopeLocked());return std::string("{\"revision\":\"GRS_M3_BOUNDED_COLD_V1\",\"state\":\"")+result+"\"}";}
void ControllerV4::BuildCold(uint64_t now){
 int slot;size_t words=0;uint32_t* out=nullptr;QuotaSnapshotV3 quota;std::unique_ptr<uint32_t[]> old;
 {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.Maintain(now,ColdScopeLocked());slot=coldTrace_.Start();if(slot<0)return;words=coldTrace_.slots[slot].requestWords;old=coldTrace_.TakeOld(slot);quota=clock_.SnapshotLocked(now,queue_.SizeLocked()!=0,run_);}
 old.reset();
 {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.OldFreed(slot);}
 std::unique_ptr<uint32_t[]> allocated;
 {std::lock_guard<std::mutex> lock(mutex_);if(coldTrace_.slots[slot].reserved)words=coldTrace_.slots[slot].reserved;else words=0;}
 if(words)allocated.reset(new(std::nothrow)uint32_t[words]);
 {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.Adopt(slot,std::move(allocated));out=coldTrace_.slots[slot].block.get();words=coldTrace_.slots[slot].capacity;}
 size_t state=0,events=0;const bool copied=out&&transport_.CaptureColdV4(out,words,quota,state,events);
 {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.Finish(slot,ColdScopeLocked(),SteadyNow(),copied?"OK":"CAPTURE_FAILED",state,events,transport_.FirstGlError(),transport_.FirstEglError()==EGL_SUCCESS?0:uint32_t(transport_.FirstEglError()),copied?"":"M3_GPU_SNAPSHOT");if(coldTrace_.published==slot&&coldTrace_.slots[slot].state==coldtrace::TraceManager::READY){transport_.TraceFrozen();run_.workspaceRetired=true;}}
}
ControllerV4::RootDiagnostic ControllerV4::DiagnosticOwnerThread()const{std::lock_guard<std::mutex> lock(mutex_);return {active_,transport_.Tail(),transport_.Evidence(),clock_.fault};}
}
