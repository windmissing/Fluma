#pragma once
#include "../cold_trace.h"
#include "../cold_diagnostics.h"
#include "../resource_ledger.h"
#include "../program_cache/artifact_store.h"
#include <condition_variable>
#include <thread>
#include <chrono>
#include <atomic>
namespace grs::resource538 {
// Only this separate module defines cache faults. No production App is linked.
class Lab {
 struct Row {std::array<char,256> raw{};};static_assert(sizeof(Row)==256);
 std::mutex mutex;std::condition_variable cv;std::thread worker;bool cancelled=false,released=false,running=false;
 uint64_t serial=0,job=0;size_t count=0,dropped=0;std::array<Row,256> records{};
 cold503::TraceManager trace;cold503::Scope scope{};resource501::Ledger ledger;std::string directory;
 void Log(const char* stage,const char* result,uint64_t a=0,uint64_t b=0){std::lock_guard<std::mutex> lock(mutex);if(count==records.size()){++dropped;return;}auto& r=records[count++];cold503::Writer w(r.raw.data(),r.raw.size());w.Raw("{\"stage\":");w.String(stage,std::min<size_t>(48,std::strlen(stage)));w.Raw(",\"result\":");w.String(result,std::min<size_t>(64,std::strlen(result)));w.Raw(",\"ns\":");w.U64(BridgeTimeline::Now());w.Raw(",\"a\":");w.U64(a);w.Raw(",\"b\":");w.U64(b);w.Raw("}");if(!w.Good()||w.Size()>=r.raw.size()){std::strcpy(r.raw.data(),"{\"state\":\"INCOMPLETE_RECORD\"}");++dropped;}else r.raw[w.Size()]=0;}
 void SaveBytes(const char* name,const char* bytes,size_t size){if(size>cold503::DiagnosticBytes){Log("FILE","UTF8_LIMIT");return;}std::filesystem::create_directories(directory);const auto path=std::filesystem::path(directory)/name;FILE* f=std::fopen(path.c_str(),"wb");if(!f){Log("FILE","OPEN_FAILED",errno);return;}const auto wrote=std::fwrite(bytes,1,size,f);const bool ok=std::fflush(f)==0&&fsync(fileno(f))==0;const auto closed=std::fclose(f);Log("FILE",name,wrote,ok&&!closed&&wrote==size);p011gpu::Sha256 hash;hash.update(bytes,size);const auto digest=hash.finish();Log("FILE_SHA256",programcache::Hex(digest.data(),digest.size()).c_str());}
 void Storage(programcache::ProgramArtifactStore& store,const char* name){const auto raw=store.Diagnostics502();SaveBytes(name,raw.data(),raw.size());}
 bool Stop(){std::lock_guard<std::mutex> lock(mutex);return cancelled;}
 void Wait(){std::unique_lock<std::mutex> lock(mutex);cv.wait(lock,[&]{return cancelled||released;});}
 uint64_t Build(uint32_t k,uint32_t kw,bool source,uint64_t replace=0,const char* finish="OK"){
  const size_t state=16*k+16*kw+2304,rows=source?512:256,stride=source?128:64;
  // SOURCE rows contain (2*k+1) records; Q maximum includes every dispatch.
  const size_t events=source?size_t(128)*(2*k+1):size_t(64)*(4*(48*k+16*kw+254)+k);
  const auto words=256+state+events;uint64_t request=0,id=0;auto now=BridgeTimeline::Now();
  const auto* result=trace.Begin(scope,scope,now,replace,words,request,id,state);Log("TRACE_BEGIN",result,trace.Capacity(),trace.Reserved());if(!id)return 0;
  const int slot=trace.Start();if(slot<0)return 0;auto old=trace.TakeOld(slot);old.reset();trace.OldFreed(slot);
  std::unique_ptr<uint32_t[]> block(new(std::nothrow) uint32_t[words]);trace.Adopt(slot,std::move(block));auto& s=trace.slots[slot];
  if(s.block){for(size_t i=0;i<words;++i)s.block[i]=uint32_t(i*2654435761u);auto* c=s.block.get();c[0]=1381192263;c[1]=3;c[2]=256;c[3]=3235823838u;c[4]=1;c[5]=0;c[6]=uint32_t(scope.control.run);c[7]=0;c[10]=uint32_t(scope.control.context);c[11]=0;c[12]=uint32_t(scope.control.surface);c[13]=0;c[27]=source?1:2;c[source?169:53]=uint32_t(events/stride);}
  trace.Finish(slot,scope,BridgeTimeline::Now(),s.block?finish:"ALLOCATION_FAILED",state,events);
  ledger.HostStorage("LAB_TRACE_SLOT",uintptr_t(&s),words*4,s.capacity*4);Log("TRACE_FINISH",s.result,trace.Capacity(),words*4);
  if(s.block){p011gpu::Sha256 sha;sha.update(s.block.get(),words*4);const auto digest=sha.finish();const auto hex=programcache::Hex(digest.data(),digest.size());Log("FIXTURE_SHA256",hex.c_str());Log("FIXTURE_RECIPE","i*2654435761u; header T445 V3 synthetic",k,kw);}return id;
 }
 void Pages(uint64_t id){cold503::TraceManager::Read read;const auto* result=trace.Acquire(id,0,BridgeTimeline::Now(),scope,read);Log("PAGE_ACQUIRE",result);if(std::strcmp(result,"OK"))return;
  std::array<char,cold503::PageBytes> bytes{};const auto pages=(256+read.stateWords+read.eventWords+cold503::PageWords-1)/cold503::PageWords;size_t maxBytes=0;uint64_t bad=0;p011gpu::Sha256 digest;
  for(size_t page=0;page<=pages&&!Stop();++page){cold503::Writer w(bytes.data(),bytes.size());cold503::TracePage(w,read,uint32_t(page));maxBytes=std::max(maxBytes,w.Size());if(!w.Good())++bad;digest.update(w.Data(),w.Size());}
  const auto sha=digest.finish();Log("PAGE_WRITER",bad?"UTF8_LIMIT":"ENCODED_SYNTHETIC",maxBytes,pages);Log("PAGE_SHA_PREFIX","SHA256",uint64_t(sha[0])<<24|uint64_t(sha[1])<<16|uint64_t(sha[2])<<8|sha[3]);trace.Release(read,BridgeTimeline::Now(),scope);
 }
 void TraceCases(const std::string& name){
  if(name=="TRACE_MAX_SOURCE"||name=="TRACE_MAX_Q"){for(auto k:{192u,512u})for(auto kw:{64u,128u}){auto id=Build(k,kw,name=="TRACE_MAX_SOURCE");Pages(id);trace.End(id,BridgeTimeline::Now(),scope);}return;}
  auto id=Build(512,128,false);
  if(name=="TWO_SLOT_PIN_REPLACE"){
   Build(512,128,false,id,"INJECTED_BUILD_FAILED");Log("FAILED_REPLACE_PRESERVES_PUBLISHED",trace.published>=0?trace.slots[trace.published].result:"NONE",trace.Capacity());
   cold503::TraceManager::Read pinned;std::thread reader([&]{const auto* r=trace.Acquire(id,0,BridgeTimeline::Now(),scope,pinned);Log("READER_BARRIER",r,trace.Capacity());cv.notify_all();Wait();});
   struct ReaderCleanup{Lab& lab;std::thread& thread;cold503::TraceManager::Read& read;~ReaderCleanup(){lab.Release();if(thread.joinable())thread.join();if(read.slot>=0)lab.trace.Release(read,BridgeTimeline::Now(),lab.scope);}} cleanup{*this,reader,pinned};
   // Publish reader pin before entering concurrent control operation.
   {std::unique_lock<std::mutex> lock(mutex);cv.wait(lock,[&]{return count&&std::strstr(records[count-1].raw.data(),"READER_BARRIER");});}
   auto newer=Build(512,128,false,id);auto third=Build(512,128,false,newer);Log("SECOND_REPLACE",third?"UNEXPECTED_ID":"WOULD_BLOCK",trace.Capacity());trace.End(newer,BridgeTimeline::Now(),scope);Log("END_PINNED","RETIRED_READER_STILL_OWNS",trace.Capacity());
   reader.join();if(pinned.slot>=0){trace.Release(pinned,BridgeTimeline::Now(),scope);pinned.slot=-1;}Log("READER_RELEASE","FREE_RETAINS_CAPACITY",trace.Capacity());return;
  }
  if(name=="TTL_WALL"){
   const auto start=trace.slots[trace.published].created;while(!Stop()){const auto now=BridgeTimeline::Now();trace.Maintain(now,scope);cold503::TraceManager::Read read;const auto* observed=trace.Acquire(id,0,now,scope,read);Log("TTL_POLL_NO_RENEW",observed,now-start);if(read.slot>=0)trace.Release(read,now,scope);if(now-start>=cold503::TtlNs){Log("TTL_WALL","EXPIRED",now-start,trace.Capacity());break;}std::unique_lock<std::mutex> lock(mutex);cv.wait_for(lock,std::chrono::seconds(1),[&]{return cancelled;});}
   Pages(id);return;
  }
  const auto created=trace.slots[trace.published].created;trace.Maintain(created+cold503::TtlNs-1,scope);Log("INJECTED_CLOCK","PRE_EDGE",trace.published>=0);trace.Maintain(created+cold503::TtlNs,scope);Log("INJECTED_CLOCK","AT_EDGE",trace.published>=0);trace.requestSerial=UINT64_MAX;uint64_t req=0,next=0;Log("ID_LIMIT",trace.Begin(scope,scope,created+cold503::TtlNs,0,1024,req,next,256));
  trace.requestSerial=0;auto reverse=Build(192,64,true);if(reverse){auto& s=trace.slots[trace.published];trace.Maintain(s.created-1,scope);Log("CLOCK_REVERSE",s.result);}
 }
 void Diagnostic(const std::string& name){
  cold503::Gate gate;Log("DIAGNOSTIC_GATE",gate.Begin(scope,scope,cold503::DiagnosticBytes));Log("DIAGNOSTIC_GATE_BUSY",gate.Begin(scope,scope,cold503::DiagnosticBytes));
  auto capture=std::unique_ptr<diagnostic492::Capture>(new(std::nothrow) diagnostic492::Capture);if(!capture){Log("CAPTURE","ALLOCATION_FAILED");gate.End();return;}
  for(size_t g=0;g<4;++g){auto s=scope.control;s.run+=g;diagnostic492::Root root;root.scope=s;root.nonce=g+1;root.kind=2;capture->Intent(root);diagnostic492::Point p;p.scope=s;p.root=g+1;std::strcpy(p.stage.data(),"FIXTURE");capture->Fault(p,"FIXTURE_FIRST_CAUSE",root);auto& group=capture->groups[g];group.count=64;group.quantumCount=64;group.tailCount=64;for(auto& point:group.first)point=p;for(auto& point:group.firstQuantum)point=p;for(auto& point:group.tail)point=p;}
  ++gate.copied;Log("DIAGNOSTIC_COPY","SINGLE_CAPTURE",gate.copied);cold503::SmallDiagnostic summary;summary.Copy(*capture,scope);auto bytes=std::unique_ptr<char[]>(new char[cold503::DiagnosticBytes]);cold503::Writer w(bytes.get(),cold503::DiagnosticBytes);cold503::DiagnosticJson(w,*capture,summary,cold503::DiagnosticBytes,false);const auto size=w.Size();bytes[size< cold503::DiagnosticBytes?size:cold503::DiagnosticBytes-1]=0;Log("DIAGNOSTIC_WRITER","ACTUAL_BYTES",size,sizeof(*capture));SaveBytes("diagnostic-complete.json",w.Data(),w.Size());
  for(auto limit:{size,size?size-1:0,cold503::DiagnosticBytes}){w.Reset();cold503::Writer edge(bytes.get(),limit);cold503::DiagnosticJson(edge,*capture,summary,limit,false);if(!edge.Good()){cold503::Writer fallback(bytes.get(),cold503::PageBytes);cold503::DiagnosticFallback(fallback,summary,"INJECTED_CREDIT",limit);Log("FALLBACK",fallback.Good()?"COMPLETE_JSON":"FAILED",fallback.Size(),limit);}else Log("CREDIT",std::string(edge.Data(),std::min<size_t>(edge.Size(),256)).find("INCOMPLETE")!=std::string::npos?"INCOMPLETE":"COMPLETE",edge.Size(),limit);}
  for(auto& group:capture->groups){group.count=768;group.quantumCount=768;for(auto* points:{&group.first,&group.firstQuantum})for(auto& p:*points)std::memset(p.stage.data(),1,p.stage.size()-1);}
  summary.Copy(*capture,scope);cold503::Writer worst(bytes.get(),std::min<size_t>(cold503::DiagnosticBytes,3*1024*1024));cold503::DiagnosticJson(worst,*capture,summary,worst.Capacity(),false);bytes[worst.Size()<cold503::DiagnosticBytes?worst.Size():cold503::DiagnosticBytes-1]=0;SaveBytes("diagnostic-worst-escape.json",worst.Data(),worst.Size());Log("WORST_ESCAPE_CLAMP",std::strstr(bytes.get(),"INCOMPLETE")?"FALLBACK":"COMPLETE",worst.Size(),worst.Capacity());
  if(name=="DIAGNOSTIC_BUSY_FAIL")for(const auto* fault:{"ALLOCATION_FAILED","NAPI_FAILED","IO_FAILED","CANCELLED","FINAL_MANIFEST_FAILED"}){cold503::Writer small(bytes.get(),cold503::PageBytes);cold503::DiagnosticFallback(small,summary,fault,cold503::PageBytes);Log("INJECTED_FAILURE",fault,small.Size(),small.Good());SaveBytes((std::string(fault)+".json").c_str(),small.Data(),small.Size());}
  gate.End();
 }
 void Cache(){
  programcache::ProgramArtifactStore store;if(!store.Configure(directory)){Log("CACHE_CONFIGURE","FAILED",errno);return;}
  const std::string key(64,'a');store.SetWriteFault(1);auto a=store.Put(key,1,std::vector<uint8_t>(4096,37));store.Drain();Log("CACHE_ENOSPC",a&&a->state==programcache::WRITE_FAILED?"WRITE_FAILED":"OBSERVED_OTHER",store.Count());Storage(store,"cache-enospc.json");a.reset();store.Reject(key);store.Drain();
  store.SetWriteFault(3);store.SetRemoveFault(true);auto b=store.Put(std::string(64,'b'),1,std::vector<uint8_t>(4096,42));store.Drain();Log("CACHE_TMP","SYNC_REMOVE_INJECTED",store.Count());Storage(store,"cache-temp-remove.json");
  store.SetWriteFault(0);store.SetRemoveFault(false);store.Put(std::string(64,'c'),1,std::vector<uint8_t>(4096,43));store.Drain();Log("CACHE_RECOVER","DRAINED",store.Count());Storage(store,"cache-recovered.json");
  std::array<std::shared_ptr<programcache::Artifact>,32> pins;for(unsigned i=0;i<32&&!Stop();++i){auto k=programcache::Hash(std::to_string(i));pins[i]=store.Put(k,1,std::vector<uint8_t>(4096,uint8_t(i)));}store.Drain();Log("CACHE_PIN_BOUNDARY",store.Put(programcache::Hash("overflow"),1,std::vector<uint8_t>(4096,1))?"ADMITTED":"PIN_REJECTED",store.Count());
  for(auto& pin:pins)pin.reset();b.reset();store.Drain();
  const auto cache=std::filesystem::path(directory)/"gpu-program-cache/v1",moved=std::filesystem::path(directory)/"lab-scan-hidden";std::error_code ec;std::filesystem::rename(cache,moved,ec);if(!ec){store.Put(programcache::Hash("scan"),1,std::vector<uint8_t>(4096,8));store.Drain();Log("CACHE_SCAN","DIRECTORY_UNAVAILABLE");Storage(store,"cache-scan-failed.json");std::filesystem::rename(moved,cache,ec);Log("CACHE_SCAN_RESTORE",ec?"FAILED":"RESTORED",ec.value());}
  std::mutex barrier;std::condition_variable wake;bool entered=false,release=false;store.Enqueue([&]{std::unique_lock<std::mutex> lock(barrier);entered=true;wake.notify_all();wake.wait(lock,[&]{return release;});});{std::unique_lock<std::mutex> lock(barrier);wake.wait(lock,[&]{return entered;});}
  unsigned queued=0;try{for(unsigned i=0;i<129;++i){store.Enqueue([]{});++queued;}}catch(const std::exception& e){Log("CACHE_QUEUE",e.what(),queued);} {std::lock_guard<std::mutex> lock(barrier);release=true;}wake.notify_all();store.Drain();Storage(store,"cache-queue-boundary.json");
  std::array<std::shared_ptr<programcache::Artifact>,4> large;for(unsigned i=0;i<4&&!Stop();++i){large[i]=store.Put(programcache::Hash("large"+std::to_string(i)),1,std::vector<uint8_t>(programcache::kEntryLimit,9));store.Drain();Log("CACHE_32M_ENTRY",large[i]?"ADMITTED":"REJECTED",store.Count(),programcache::kEntryLimit);}
  auto overflow=store.Put(programcache::Hash("capacity"),1,std::vector<uint8_t>(1,1));Log("CACHE_128M_PINNED",overflow?"ADMITTED":"REJECTED",store.Count());Storage(store,"cache-128m-pinned.json");for(auto& a:large)a.reset();store.Drain();
  auto oversize=store.Put(programcache::Hash("entry-over"),1,std::vector<uint8_t>(programcache::kEntryLimit+1,1));Log("CACHE_ENTRY_LIMIT",oversize?"ADMITTED":"REJECTED",programcache::kEntryLimit+1);
  auto diagnostics=store.Diagnostics502();p011gpu::Sha256 h;h.update(diagnostics.data(),diagnostics.size());auto digest=h.finish();Log("CACHE_STORAGE_SHA256",programcache::Hex(digest.data(),digest.size()).c_str());
  std::filesystem::path receipt=std::filesystem::path(directory)/"cache-storage.json";FILE* file=std::fopen(receipt.c_str(),"wb");if(file){const auto n=std::fwrite(diagnostics.data(),1,diagnostics.size(),file);const auto sync=std::fflush(file)==0&&fsync(fileno(file))==0;std::fclose(file);Log("CACHE_STORAGE_FILE",sync&&n==diagnostics.size()?"cache-storage.json":"IO_FAILED",n);}

 }
 void Run(std::string name){try{Log("START",name.c_str());if(name.find("TRACE_")==0||name=="TWO_SLOT_PIN_REPLACE"||name.find("TTL_")==0)TraceCases(name);else if(name.find("DIAGNOSTIC_")==0)Diagnostic(name);else if(name=="CACHE_FAULT")Cache();else Log("RESOURCE_PHASE_SAMPLE","USE_PRODUCTION_READ_ONLY_PAGE");for(int i=0;i<2;++i){if(trace.slots[i].state!=cold503::TraceManager::FREE)trace.End(trace.slots[i].id,BridgeTimeline::Now(),scope);auto block=trace.Trim(i);block.reset();ledger.HostStorage("LAB_TRACE_SLOT",uintptr_t(&trace.slots[i]),0,0,false);}Log("END",Stop()?"CANCELLED":"COMPLETED_FIXTURE_NOT_PHYSICS",trace.Capacity());}catch(const std::exception& e){Log("FAIL",e.what());}for(int i=0;i<2;++i){if(trace.slots[i].state!=cold503::TraceManager::FREE)trace.End(trace.slots[i].id,BridgeTimeline::Now(),scope);auto block=trace.Trim(i);block.reset();ledger.HostStorage("LAB_TRACE_SLOT",uintptr_t(&trace.slots[i]),0,0,false);}std::lock_guard<std::mutex> lock(mutex);running=false;}
public:
 ~Lab(){Cancel();if(worker.joinable())worker.join();}
 std::string Start(const std::string& name,const std::string& files){
  static constexpr const char* allowed[]={"RESOURCE_PHASE_SAMPLE","TRACE_MAX_SOURCE","TRACE_MAX_Q","TWO_SLOT_PIN_REPLACE","TTL_WALL","TTL_EDGE_INJECTED","DIAGNOSTIC_CREDIT","DIAGNOSTIC_BUSY_FAIL","CACHE_FAULT"};bool valid=false;for(auto s:allowed)valid|=name==s;if(!valid)return "INVALID_CASE";
  {std::lock_guard<std::mutex> lock(mutex);if(running)return "LAB_BUSY";}if(worker.joinable())worker.join();
  const auto root=std::filesystem::weakly_canonical(files);const auto base=root/"t531-resource-lab"/(std::to_string(getpid())+"-"+std::to_string(BridgeTimeline::Now())+"-"+std::to_string(serial+1));if(root.empty()||!root.is_absolute()||base.lexically_relative(root).empty())return "LAB_DIRECTORY_REJECTED";
  const auto canonical=std::filesystem::weakly_canonical(base);const auto relative=canonical.lexically_relative(root);if(relative.empty()||*relative.begin()=="..")return "LAB_DIRECTORY_REJECTED";directory=canonical.string();trace={};scope={};scope.pid=getpid();scope.control.epoch=scope.control.run=scope.control.context=scope.control.surface=scope.control.session=1;std::strcpy(scope.route.data(),"m2");std::strcpy(scope.source.data(),"T531_RESOURCE_LAB_V1_FIXTURE_SYNTHETIC");
  {std::lock_guard<std::mutex> lock(mutex);job=++serial;count=dropped=0;running=true;cancelled=released=false;}try{worker=std::thread([this,name]{Run(name);});}catch(...){std::lock_guard<std::mutex> lock(mutex);running=false;return "WORKER_ALLOCATION_FAILED";}return std::to_string(job);
 }
 void Cancel(){std::lock_guard<std::mutex> lock(mutex);cancelled=true;released=true;cv.notify_all();}
 void Release(){std::lock_guard<std::mutex> lock(mutex);released=true;cv.notify_all();}
 std::string Poll(uint64_t id){std::array<char,131072> bytes{};cold503::Writer w(bytes.data(),bytes.size());std::lock_guard<std::mutex> lock(mutex);w.Raw("{\"revision\":\"T531_RESOURCE_LAB_RECORD_V1\",\"evidenceOrigin\":\"FIXTURE_SYNTHETIC\",\"physicsValidated\":false,\"currentGranted\":false,\"deviceGLMeasured\":false,\"jobId\":");w.U64(job);w.Raw(",\"state\":");w.Raw(id!=job?"\"STALE_JOB\"":running?"\"RUNNING\"":"\"FINISHED\"");w.Raw(",\"scope\":");cold503::ScopeJson(w,scope);w.Raw(",\"sessionDirectory\":");w.String(directory.data(),directory.size());w.Raw(",\"knownHostPayloadLimit\":58993832,\"excluded\":\"helper fixed objects; cache pins 128MiB; worker stacks; allocator\",\"dropped\":");w.Number(dropped);w.Raw(",\"records\":[");for(size_t i=0;i<count;++i){if(i)w.Raw(",");w.Raw(records[i].raw.data());}w.Raw("]}");return w.Good()?std::string(w.Data(),w.Size()):"{\"state\":\"INCOMPLETE\"}";}
 bool Matches(uint64_t id){std::lock_guard<std::mutex> lock(mutex);return id!=0&&id==job;}
};
}
