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
 auto drain=std::async(std::launch::async,[&]{return store.DrainForTest(deadline,cancel,&observed);});
 if(mode!="deadline"){std::this_thread::sleep_for(std::chrono::milliseconds(30));if(mode=="full_queue_pop"){std::lock_guard<std::mutex> lock(mutex);release=true;cv.notify_all();}else store.CancelDrainForTest(cancel);}
 const auto result=drain.get();if(mode=="full_queue_pop")assert(completedJobs==128);const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-begin).count();
 {std::lock_guard<std::mutex> lock(mutex);release=true;cv.notify_all();}store.RetireForTest();
 assert(result==(mode=="deadline"?Store::DrainResult::TIMEOUT:mode=="full_queue_pop"?Store::DrainResult::DRAINED:Store::DrainResult::CANCELLED));assert(observed.queuedAtEntry==(full?128:0));assert((observed.fenceSequence!=0)==(!full||mode=="full_queue_pop"));if(mode=="full_queue_pop")assert(observed.popNs>=observed.insertNs&&observed.insertNs>=observed.enterNs);
 std::cout<<"{\"mode\":\""<<mode<<"\",\"result\":\"EXPECTED\",\"queuedAtEntry\":"<<observed.queuedAtEntry<<",\"fenceSequence\":"<<observed.fenceSequence<<",\"enterNs\":\""<<observed.enterNs<<"\",\"insertNs\":\""<<observed.insertNs<<"\",\"popNs\":\""<<observed.popNs<<"\",\"returnNs\":\""<<observed.returnNs<<"\",\"elapsedMs\":"<<elapsed<<",\"retired\":true,\"origin\":\"FIXTURE_NOT_PHYSICS\"}\n";
}
#ifndef T668_STORE_ONLY
static void lab(const std::string& mode,const std::string& dir){
 grs::resource538::Lab lab;const auto id=std::stoull(lab.Start(mode=="barrier_cancel"?"CHECKPOINT_BARRIER":"RESOURCE_PHASE_SAMPLE",dir));const auto deadline=Clock::now()+std::chrono::seconds(15);bool barrier=false;std::string raw;
 while(Clock::now()<deadline){raw=lab.Poll(id);if(mode=="barrier_cancel"&&!barrier&&raw.find("BARRIER_ENTER")!=std::string::npos){barrier=true;std::cout<<lab.CancelJson(id)<<'\n';std::cout<<lab.CancelJson(id)<<'\n';}if(raw.find("\"state\":\"FINISHED\"")!=std::string::npos)break;std::this_thread::sleep_for(std::chrono::milliseconds(5));}
 if(mode=="barrier_cancel")assert(barrier);if(raw.find("\"state\":\"FINISHED\"")==std::string::npos){std::cout<<lab.CancelJson(id)<<'\n';throw std::runtime_error("LAB_TERMINAL_TIMEOUT");}
 std::cout<<raw<<'\n';std::cout<<lab.CancelJson(id)<<'\n';
 if((mode=="files"||mode=="files_ttl"||mode=="files_mutation")){
  auto begin=lab.FilesBeginV2(id);std::cout<<begin<<'\n';auto exp=number(begin,"exportId");std::cout<<lab.FilesBeginV2(id)<<'\n';std::cout<<lab.FilesEndV2(exp+1)<<'\n';
  if(mode=="files_ttl"){std::this_thread::sleep_for(std::chrono::seconds(61));auto expired=lab.FilesIndexV2(exp,0);std::cout<<expired<<'\n';assert(expired.find("EXPIRED")!=std::string::npos);return;}
  if(mode=="files_mutation"){const auto index=lab.FilesIndexV2(exp,0);std::cout<<index<<'\n';const auto name=text(index,"name");const auto directory=text(raw,"sessionDirectory");auto path=std::filesystem::path(directory)/name;if(path.parent_path()!=std::filesystem::path(directory))throw std::runtime_error("FIXTURE_PATH");std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);char byte;if(!file.read(&byte,1))throw std::runtime_error("MUTATION_EMPTY");byte^=1;file.seekp(0);file.write(&byte,1);file.flush();file.close();auto changed=lab.FilePageV2(exp,0,0);std::cout<<changed<<'\n';assert(changed.find("FILE_CHANGED")!=std::string::npos);return;}
  for(uint32_t page=0;page<4;++page){auto index=lab.FilesIndexV2(exp,page);std::cout<<index<<'\n';size_t at=0;while((at=index.find("\"fileId\":",at))!=std::string::npos){auto entry=index.substr(at);auto file=number(entry,"fileId"),bytes=number(entry,"bytes");for(uint64_t offset=0;offset<bytes;offset+=4096)std::cout<<lab.FilePageV2(exp,file,offset)<<'\n';at+=9;}if(index.find("\"done\":true")!=std::string::npos)break;}
  std::cout<<lab.FilesEndV2(exp)<<'\n';std::cout<<lab.FilesIndexV2(exp,0)<<'\n';
  for(int failure=0;failure<3;++failure){auto b=lab.FilesBeginV2(id);std::cout<<b<<'\n';auto e=number(b,"exportId");std::cout<<(failure==0?lab.FilesIndexV2(e,999):failure==1?lab.FilePageV2(e,999,0):lab.FilePageV2(e,0,1))<<'\n';lab.FilesEndV2(e);}
 }
}
#endif
int main(int argc,char** argv){try{if(argc<2)return 2;std::string mode=argv[1];if(mode=="full_queue_cancel"||mode=="full_queue_pop"||mode=="fence_cancel"||mode=="deadline")queue(mode);
#ifndef T668_STORE_ONLY
else if((mode=="barrier_cancel"||(mode=="files"||mode=="files_ttl"||mode=="files_mutation"))&&argc==3)lab(mode,argv[2]);
#endif
else return 2;return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
