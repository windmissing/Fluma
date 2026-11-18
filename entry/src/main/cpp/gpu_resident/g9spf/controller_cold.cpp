#include "controller.h"
#include <chrono>
#include <new>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
namespace grs::g9spf {
namespace {uint64_t Now(){return candidate548::ClockProducer::Now();}}
cold503::Scope Controller::ColdScopeLocked()const{
 cold503::Scope scope;
#ifdef _WIN32
 scope.pid=uint64_t(_getpid());
#else
 scope.pid=uint64_t(getpid());
#endif
 scope.control.epoch=coldEpoch_;scope.control.run=identity_.run;scope.control.context=identity_.context;scope.control.surface=identity_.surface;scope.control.session=identity_.pageSession;scope.control.reset=identity_.inputEpoch;scope.control.permission=identity_.permission;
 std::strncpy(scope.route.data(),"g9spf",scope.route.size()-1);std::memcpy(scope.source.data(),kEngineeringSha,64);std::memcpy(scope.source.data()+64,PipelineSourceHash(),64);scope.source[128]=0;return scope;
}
std::string Controller::ColdCapabilities()const{
 std::array<char,2048> data{};cold::Writer w(data.data(),data.size());
 std::lock_guard<std::mutex> lock(mutex_);cold::Layout layout;if(!configured_||!cold::Layout::Make(*profile_,4,layout,config_.fullTrialTrace))return "{\"state\":\"UNSUPPORTED_G9_COLD\"}";
 w.Raw(config_.realTouch?"{\"revision\":\"G9_SPF_COLD_V2\",\"pageRevision\":\"G9_SPF_SEGMENT_PAGE_V2\",\"route\":\"g9spf\",\"K\":" : "{\"revision\":\"G9_SPF_COLD_V1\",\"pageRevision\":\"G9_SPF_SEGMENT_PAGE_V1\",\"route\":\"g9spf\",\"K\":");w.Number(config_.k);w.Raw(",\"traceBytes\":");w.U64(layout.words*4);w.Raw(",\"traceBudgetBytes\":\"359342080\",\"pageBytes\":16384,\"pageWords\":1024,\"slots\":2,\"builders\":1,\"readers\":1,\"ttlNs\":\"60000000000\",\"pages\":");w.Number(layout.Pages());w.Raw(",\"archiveUtf8Limit\":");w.U64((layout.Pages()+1)*cold::kPageBytes);w.Raw(",\"fullTrialTrace\":");w.Bool(config_.fullTrialTrace);w.Raw(",\"source\":");w.String(ColdScopeLocked().source);w.Raw("}");return std::string(w.Data(),w.Size());
}
std::string Controller::ColdBegin(uint64_t replace,uint64_t now){
 coldtrace::TraceManager::Receipt receipt;
 {std::lock_guard<std::mutex> lock(mutex_);receipt.scope=ColdScopeLocked();cold::Layout layout;
  if(!configured_||rootLive_||transport_.Busy()||coldBuilding_||!lastGood_||!cold::Layout::Make(*profile_,4,layout,config_.fullTrialTrace))receipt.result="WOULD_BLOCK";
  else{receipt.result=coldTrace_.Begin(receipt.scope,receipt.scope,now,replace,layout.words,receipt.request,receipt.id,layout.words-256,config_.fullTrialTrace?1u:0u);if(std::strcmp(receipt.result,"REQUESTED")==0)coldBuilding_=true;}
 }
 std::array<char,16384> data{};cold::Writer w(data.data(),data.size());coldtrace::ReceiptJson(w,receipt,config_.realTouch);return std::string(w.Data(),w.Size());
}
std::string Controller::ColdState(uint64_t request,uint64_t now){
 coldtrace::TraceManager::Receipt receipt;{std::lock_guard<std::mutex> lock(mutex_);receipt=coldTrace_.StateOf(request,now,ColdScopeLocked());}
 std::array<char,16384> data{};cold::Writer w(data.data(),data.size());coldtrace::ReceiptJson(w,receipt,config_.realTouch);return std::string(w.Data(),w.Size());
}
std::string Controller::ColdEnd(uint64_t trace,uint64_t now){
 std::lock_guard<std::mutex> lock(mutex_);const auto result=coldTrace_.End(trace,now,ColdScopeLocked());return std::string(config_.realTouch?"{\"revision\":\"G9_SPF_COLD_V2\",\"state\":\"" : "{\"revision\":\"G9_SPF_COLD_V1\",\"state\":\"")+result+"\"}";
}
std::string Controller::ColdPage(uint64_t trace,uint32_t page,uint64_t now){
 coldtrace::TraceManager::Read read;const char* result;
 {std::lock_guard<std::mutex> lock(mutex_);result=coldTrace_.Acquire(trace,page,now,ColdScopeLocked(),read);}
 if(std::strcmp(result,"OK"))return std::string("{\"state\":\"")+result+"\"}";
 struct ReleaseRead {Controller* self;coldtrace::TraceManager::Read read;bool active=true;~ReleaseRead(){if(active){std::lock_guard<std::mutex> lock(self->mutex_);self->coldTrace_.Release(read,Now(),self->ColdScopeLocked());}}} release{this,read};
 std::array<char,16384> data{};cold::Writer w(data.data(),data.size());coldtrace::TracePage(w,read,page);
 {std::lock_guard<std::mutex> lock(mutex_);result=coldTrace_.Release(read,Now(),ColdScopeLocked());release.active=false;}
 if(!w.Good()||std::strcmp(result,"TRACE_READY"))return "{\"state\":\"INCOMPLETE_SCOPE_CHANGED\"}";
 return std::string(w.Data(),w.Size());
}
void Controller::BuildCold(uint64_t now){
 int slot;size_t words=0;QuotaSnapshotV3 quota;std::unique_ptr<uint32_t[]> detached;bool full=false;
 {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.Maintain(now,ColdScopeLocked());slot=coldTrace_.Start();if(slot<0){if(coldTrace_.builder<0)coldBuilding_=false;return;}words=coldTrace_.slots[slot].requestWords;detached=coldTrace_.TakeOld(slot);quota=clock_.SnapshotLocked(now,size_&&queue_[head_].input[40]==1,run_);full=config_.fullTrialTrace;}
 detached.reset();{std::lock_guard<std::mutex> lock(mutex_);coldTrace_.OldFreed(slot);}
 std::unique_ptr<uint32_t[]> allocated;
 {std::lock_guard<std::mutex> lock(mutex_);words=coldTrace_.slots[slot].reserved;}
 if(words)allocated.reset(new(std::nothrow)uint32_t[words]);
 uint32_t* storage=nullptr;uint64_t request=0,trace=0,created=0;
 {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.Adopt(slot,std::move(allocated));auto& s=coldTrace_.slots[slot];storage=s.block.get();words=s.capacity;request=s.request;trace=s.id;created=s.created;}
 // coldBuilding_ pins both root-bound records; new roots cannot overwrite them.
 const bool copied=storage&&transport_.CaptureSpan(storage,words,full,quota.data(),request,trace,created,&frozenTouch_,&baseTouch_);
 {std::lock_guard<std::mutex> lock(mutex_);coldTrace_.Finish(slot,ColdScopeLocked(),Now(),copied?"OK":"G9_CAPTURE_FAILED",coldTrace_.slots[slot].requestWords-256,0,transport_.ColdGl(),transport_.ColdEgl(),copied?"":"G9_OWNER_CAPTURE");coldBuilding_=false;}
}
}
