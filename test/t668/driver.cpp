#define GRS_PROGRAM_CACHE_TEST_FAULTS
#include "program_cache/artifact_store.h"
#ifndef T668_STORE_ONLY
#include "resource_lab/lab.h"
#endif
#include <iostream>
#include <fstream>
#include <cassert>
using Store=grs::programcache::ProgramArtifactStore;
using Clock=std::chrono::steady_clock;
static uint64_t number(const std::string& s,const std::string& key){auto at=s.find("\""+key+"\":");if(at==std::string::npos)throw std::runtime_error("MISSING_"+key);at+=key.size()+3;if(s[at]=='\"')++at;return std::stoull(s.substr(at));}
static std::string text(const std::string& raw,const std::string& key){auto at=raw.find("\""+key+"\":\"");if(at==std::string::npos)throw std::runtime_error("MISSING_STRING_"+key);at+=key.size()+4;auto end=raw.find('"',at);auto value=raw.substr(at,end-at);if(value.find('\\')!=std::string::npos)throw std::runtime_error("ESCAPED_PATH_UNSUPPORTED");return value;}
static void queue(const std::string& mode){
 Store store;std::atomic<int> completedJobs{0};std::mutex mutex;std::condition_variable cv;bool entered=false,release=false;
 store.Enqueue([&]{std::unique_lock<std::mutex> lock(mutex);entered=true;cv.notify_all();cv.wait(lock,[&]{return release;});});
 {std::unique_lock<std::mutex> lock(mutex);if(!cv.wait_for(lock,std::chrono::seconds(1),[&]{return entered;}))throw std::runtime_error("BARRIER_ENTER_TIMEOUT");}
 const bool full=mode!="fence_cancel";if(full){for(int i=0;i<128;++i)store.Enqueue([&]{++completedJobs;});bool rejected=false;try{store.Enqueue([]{});}catch(const std::runtime_error& e){rejected=std::string(e.what())=="CACHE_QUEUE_FULL";}assert(rejected);}

 std::atomic<bool> cancel{false};Store::DrainObservation observed;const auto begin=Clock::now();const auto deadline=begin+std::chrono::seconds(5);
 std::string stage;Store::DrainObservation gate;
 auto drain=std::async(std::launch::async,[&]{
   return store.DrainForTest(deadline,cancel,&observed,[&](const char* name,const auto& sample){
     std::lock_guard<std::mutex> lock(mutex);
     if(stage.empty()){stage=name;gate=sample;}cv.notify_all();
   });
 });
 bool reached=false;
 {
   std::unique_lock<std::mutex> lock(mutex);
   reached=cv.wait_until(lock,deadline,[&]{return !stage.empty();});
   if(!reached||mode=="full_queue_pop"){release=true;cv.notify_all();}
 }
 if(mode!="deadline"&&mode!="full_queue_pop")store.CancelDrainForTest(cancel);
 if(!reached){drain.wait();store.RetireForTest();throw std::runtime_error("DRAIN_PHASE_TIMEOUT");}
 assert(stage==(full?"WAIT_SPACE":"WAIT_FENCE"));
 assert((gate.fenceSequence>0)==!full);
 std::cout<<"{\"stage\":\""<<stage<<"\",\"deadlineNs\":\""<<deadline.time_since_epoch().count()<<"\",\"entryNs\":\""<<gate.enterNs<<"\",\"insertNs\":\""<<gate.insertNs<<"\"}\n";
 const auto result=drain.get();if(mode=="full_queue_pop")assert(completedJobs==128);const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-begin).count();
 {std::lock_guard<std::mutex> lock(mutex);release=true;cv.notify_all();}store.RetireForTest();
 assert(result==(mode=="deadline"?Store::DrainResult::TIMEOUT:mode=="full_queue_pop"?Store::DrainResult::DRAINED:Store::DrainResult::CANCELLED));assert(observed.queuedAtEntry==(full?128:0));assert((observed.fenceSequence!=0)==(!full||mode=="full_queue_pop"));if(mode=="full_queue_pop")assert(observed.popNs>=observed.insertNs&&observed.insertNs>=observed.enterNs);
 std::cout<<"{\"mode\":\""<<mode<<"\",\"result\":\"EXPECTED\",\"queuedAtEntry\":"<<observed.queuedAtEntry<<",\"fenceSequence\":"<<observed.fenceSequence<<",\"enterNs\":\""<<observed.enterNs<<"\",\"insertNs\":\""<<observed.insertNs<<"\",\"popNs\":\""<<observed.popNs<<"\",\"returnNs\":\""<<observed.returnNs<<"\",\"elapsedMs\":"<<elapsed<<",\"retired\":true,\"origin\":\"FIXTURE_NOT_PHYSICS\"}\n";
}
#ifndef T668_STORE_ONLY
static bool mutation(const std::string& mode) {
 return mode=="files_mutation"||mode=="files_mutation_truncate"||
        mode=="files_mutation_replace"||mode=="files_mutation_delete";
}
static void lab(const std::string& mode,const std::string& dir){
 const std::filesystem::path root(dir);
 if(!root.is_absolute()||!std::filesystem::is_directory(root)||!std::filesystem::is_empty(root))
   throw std::runtime_error("REQUIRE_NEW_ABSOLUTE_FIXTURE_DIRECTORY");
 const std::string scenario=mode.rfind("barrier_",0)==0?"CHECKPOINT_BARRIER":
   mode=="injected_clock"?"TTL_EDGE_INJECTED":mode=="injected_fault"?"CACHE_FAULT":
   mode=="catalog_contention"?"TWO_SLOT_PIN_REPLACE":"RESOURCE_PHASE_SAMPLE";
 std::cout<<"{\"scenario\":\""<<scenario<<"\",\"origin\":\""<<
   (mode=="injected_clock"?"INJECTED_CLOCK":mode=="injected_fault"?"INJECTED_FAULT":"FIXTURE_SYNTHETIC")<<"\"}\n";
 grs::resource538::Lab lab;const auto id=std::stoull(lab.Start(scenario,dir));const auto deadline=Clock::now()+std::chrono::seconds(15);bool barrier=false;std::string raw,lastState;
 while(Clock::now()<deadline){raw=lab.Poll(id);const auto state=text(raw,"state");if(state!=lastState&&state!="FINISHED"){std::cout<<raw<<'\n';lastState=state;}if(mode.rfind("barrier_",0)==0&&!barrier&&raw.find("BARRIER_ENTER")!=std::string::npos){
  barrier=true;std::cout<<raw<<'\n';
  if(mode=="barrier_release"){lab.Release();std::cout<<"{\"action\":\"RELEASE_AFTER_BARRIER_ENTER\"}\n";}
  else {auto first=lab.CancelJson(id),retry=lab.CancelJson(id);std::cout<<first<<'\n'<<retry<<'\n';
   assert(text(first,"cancelResult")=="REQUESTED");
   const auto result=text(retry,"cancelResult");assert(result=="ALREADY_REQUESTED"||result=="ALREADY_TERMINAL");}
 }if(raw.find("\"state\":\"FINISHED\"")!=std::string::npos)break;std::this_thread::sleep_for(std::chrono::milliseconds(5));}
 if(mode.rfind("barrier_",0)==0)assert(barrier);if(raw.find("\"state\":\"FINISHED\"")==std::string::npos){std::cout<<lab.CancelJson(id)<<'\n';throw std::runtime_error("LAB_TERMINAL_TIMEOUT");}
 std::cout<<raw<<'\n';
 const auto wrong=lab.CancelJson(id+1),terminal=lab.CancelJson(id),repeat=lab.CancelJson(id),after=lab.Poll(id);
 std::cout<<wrong<<'\n'<<terminal<<'\n'<<repeat<<'\n'<<after<<'\n';
 assert(text(wrong,"cancelResult")=="STALE_JOB"&&text(terminal,"cancelResult")=="ALREADY_TERMINAL"&&text(repeat,"cancelResult")=="ALREADY_TERMINAL");
 for(const char* key:{"terminalNs","jobId","generation","cancelRequestedNs","cancelObservedNs"})assert(number(raw,key)==number(after,key));
 for(const char* key:{"outcome","firstFailure","recordsSHA256","pendingResources","state"})assert(text(raw,key)==text(after,key));
 assert(text(after,"pendingResources")=="NONE"&&number(after,"pollSampleNs")>=number(after,"terminalNs"));
 if(mode=="barrier_release")assert(text(after,"outcome")=="COMPLETED_FIXTURE");
 if(mode=="barrier_cancel")assert(number(after,"cancelRequestedNs")&&number(after,"cancelObservedNs")>=number(after,"cancelRequestedNs"));
 if((mode=="files"||mode=="files_ttl"||mutation(mode))){
  const auto started=Clock::now();auto begin=lab.FilesBeginV2(id);std::cout<<begin<<'\n';auto exp=number(begin,"exportId");const auto busy=lab.FilesBeginV2(id),wrongEnd=lab.FilesEndV2(exp+1);
  std::cout<<busy<<'\n'<<wrongEnd<<'\n';
  assert(text(busy,"state")=="LAB_EXPORT_BUSY"&&text(wrongEnd,"state")=="STALE_EXPORT");
  const auto liveIndex=lab.FilesIndexV2(exp,0);assert(text(liveIndex,"state")=="OK");
  if(mode=="files_ttl"){std::this_thread::sleep_until(started+std::chrono::seconds(10));auto live=lab.FilesIndexV2(exp,0);std::cout<<live<<'\n';assert(number(live,"exportId")==exp);std::this_thread::sleep_until(started+std::chrono::seconds(61));auto expired=lab.FilesIndexV2(exp,0);std::cout<<expired<<'\n';assert(expired.find("EXPIRED")!=std::string::npos);return;}
  if(mutation(mode)){const auto index=lab.FilesIndexV2(exp,0);std::cout<<index<<'\n';const auto name=text(index,"name");const auto directory=text(raw,"sessionDirectory");auto path=std::filesystem::path(directory)/name;if(path.parent_path()!=std::filesystem::path(directory))throw std::runtime_error("FIXTURE_PATH");const auto originalSize=std::filesystem::file_size(path);if(!originalSize)throw std::runtime_error("EMPTY_FIXTURE_FILE");
   auto fileSHA=[&]{std::ifstream input(path,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(input)),{});return grs::programcache::Hash(bytes.data(),bytes.size());};
   const auto priorSHA=fileSHA();
   if(mode=="files_mutation_delete")std::filesystem::remove(path);
   else if(mode=="files_mutation_truncate")std::filesystem::resize_file(path,originalSize-1);
   else if(mode=="files_mutation_replace"){
     auto replacement=path;replacement+=".replacement";
     std::filesystem::copy_file(path,replacement);std::filesystem::rename(replacement,path);
   }else{
   std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);char byte;if(!file.read(&byte,1))throw std::runtime_error("MUTATION_EMPTY");byte^=1;file.seekp(0);file.write(&byte,1);file.flush();file.close();}
   const auto afterSHA=std::filesystem::exists(path)?fileSHA():"ABSENT";
   std::cout<<"{\"beforeSHA256\":\""<<priorSHA<<"\",\"afterSHA256\":\""<<afterSHA<<"\"}\n";
   auto changed=lab.FilePageV2(exp,0,0);std::cout<<changed<<'\n';assert(changed.find("FILE_CHANGED")!=std::string::npos);auto reopened=lab.FilesBeginV2(id);std::cout<<reopened<<'\n';assert(reopened.find("BUSY")==std::string::npos);return;}
  for(uint32_t page=0;page<4;++page){auto index=lab.FilesIndexV2(exp,page);std::cout<<index<<'\n';size_t at=0;while((at=index.find("\"fileId\":",at))!=std::string::npos){auto entry=index.substr(at);auto file=number(entry,"fileId"),bytes=number(entry,"bytes");for(uint64_t offset=0;offset<bytes;offset+=4096)std::cout<<lab.FilePageV2(exp,file,offset)<<'\n';at+=9;}if(index.find("\"done\":true")!=std::string::npos)break;}
  const auto ended=lab.FilesEndV2(exp),stale=lab.FilesIndexV2(exp,0);
  std::cout<<ended<<'\n'<<stale<<'\n';
  assert(text(ended,"state")=="OK"&&text(stale,"state")=="STALE_EXPORT");
  for(int failure=0;failure<3;++failure){auto b=lab.FilesBeginV2(id);std::cout<<b<<'\n';auto e=number(b,"exportId");const auto rejected=failure==0?lab.FilesIndexV2(e,999):failure==1?lab.FilePageV2(e,999,0):lab.FilePageV2(e,0,1);
   std::cout<<rejected<<'\n';assert(text(rejected,"state")== (failure==0?"INVALID_PAGE":failure==1?"INVALID_FILE":"INVALID_OFFSET"));
   const auto closed=lab.FilesEndV2(e);std::cout<<closed<<'\n';assert(text(closed,"state")=="OK");}
 }
}
#endif
int main(int argc,char** argv){try{if(argc<2)return 2;std::string mode=argv[1];if(mode=="full_queue_cancel"||mode=="full_queue_pop"||mode=="fence_cancel"||mode=="deadline")queue(mode);
#ifndef T668_STORE_ONLY
else if((mode=="barrier_cancel"||mode=="barrier_release"||mode=="injected_clock"||mode=="injected_fault"||mode=="catalog_contention"||(mode=="files"||mode=="files_ttl"||mutation(mode)))&&argc==3)lab(mode,argv[2]);
#endif
else return 2;return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
