#define GRS_PROGRAM_CACHE_TEST_FAULTS
#include "../../entry/src/main/cpp/gpu_resident/program_cache/artifact_store.h"
#include <cassert>
#include <iostream>
using namespace grs::programcache;
namespace fs=std::filesystem;
uint64_t metric(ProgramArtifactStore& s,const char* name){auto json=s.Diagnostics502();const auto key="\""+std::string(name)+"\":";auto i=json.find(key);assert(i!=std::string::npos);return std::stoull(json.substr(i+key.size()));}
std::vector<uint8_t> read(const fs::path& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};}
void sparse(const fs::path& p,uint64_t bytes){std::ofstream(p).put('x');fs::resize_file(p,bytes);}
int main(int argc,char** argv){assert(argc==2||argc==3);const fs::path root=argv[1];fs::create_directories(root);unsigned passed=0;std::ofstream evidence(root/"metrics.ndjson");auto observe=[&](ProgramArtifactStore& store){evidence<<store.Diagnostics502()<<std::endl;};
 if(argc==3){ProgramArtifactStore store;assert(store.Configure(root));const auto key=Hash("separate-process");if(std::string(argv[2])=="seed"){store.SetWriteFault(3);store.SetRemoveFault(true);auto a=store.Put(key,7,{1,2,3});store.Drain();assert(a&&a->state==WRITE_FAILED&&metric(store,"tempBytes")==147);}else{assert(metric(store,"tempBytes")==0&&!store.Get(key));auto a=store.Put(key,7,{1,2,3});store.Drain();assert(a&&a->state==PERSISTED);}observe(store);std::cout<<"{\"revision\":\"T502_PROCESS_RESTART_V1\",\"phase\":\""<<argv[2]<<"\",\"passed\":true}"<<std::endl;return 0;}
 const auto key=Hash("valid-old"),next=Hash("write-failed");
 {
  ProgramArtifactStore store;assert(store.Configure(root/"cleanup"));auto old=store.Put(key,7,{1,2,3});store.Drain();assert(old&&old->state==PERSISTED);const auto dir=root/"cleanup/gpu-program-cache/v1";const auto original=read(dir/(key+".bin"));
  store.SetRemoveFault(true);store.SetWriteFault(3);auto failed=store.Put(next,7,std::vector<uint8_t>(1024,8));store.Drain();assert(failed&&failed->state==WRITE_FAILED);assert(metric(store,"tempBytes")==1168&&metric(store,"tempFiles")==1&&metric(store,"cleanupFailures")>0);assert(read(dir/(key+".bin"))==original);observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
  // Repeated same-key receipt writes cannot truncate or grow its undeletable temp.
  for(unsigned i=0;i<12;++i)store.Receipt(key,"{\"stage\":\"FAULT\"}",old);store.Drain();const auto remaining=metric(store,"tempBytes");assert(remaining>=1168&&metric(store,"tempFiles")==2);assert(read(dir/(key+".bin"))==original);observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
  store.SetWriteFault(0);store.SetRemoveFault(false);store.Receipt(key,"{\"stage\":\"RECOVERED\"}",old);store.Drain();assert(metric(store,"tempBytes")==0&&metric(store,"tempFiles")==0&&old->state==PERSISTED);observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
 }
 {
  const auto dir=root/"budget/gpu-program-cache/v1";fs::create_directories(dir);sparse(dir/(Hash("residual")+".bin.tmp"),kTotalLimit-512);
  ProgramArtifactStore store;store.SetRemoveFault(true);assert(store.Configure(root/"budget"));assert(metric(store,"tempBytes")==kTotalLimit-512);
  auto a=store.Put(key,7,std::vector<uint8_t>(1024,1));store.Drain();assert(a&&a->state==WRITE_FAILED&&!fs::exists(dir/(key+".bin")));assert(metric(store,"diskRoomRejected")>0);observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
  store.SetRemoveFault(false);store.Receipt(key,"{\"stage\":\"RECOVERED\"}",a);store.Drain();assert(metric(store,"tempBytes")==0);observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
 }
 {
  const auto dir=root/"keys/gpu-program-cache/v1";fs::create_directories(dir);for(unsigned i=0;i<32;++i)sparse(dir/(Hash("tmp"+std::to_string(i))+".bin.tmp"),1);
  ProgramArtifactStore store;store.SetRemoveFault(true);assert(store.Configure(root/"keys"));auto a=store.Put(key,7,{1});store.Drain();assert(a&&a->state==WRITE_FAILED&&metric(store,"tempFiles")==32&&metric(store,"diskKeysLastReservation")==33);observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
 }
 {
  const auto dir=root/"startup/gpu-program-cache/v1";fs::create_directories(dir);sparse(dir/(key+".bin.tmp"),kTotalLimit+1ull);
  ProgramArtifactStore store;store.SetRemoveFault(true);assert(!store.Configure(root/"startup"));assert(metric(store,"tempBytes")==kTotalLimit+1ull);observe(store);store.SetRemoveFault(false);assert(store.Configure(root/"startup"));assert(metric(store,"tempBytes")==0);observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
 }
 {
  ProgramArtifactStore store;assert(store.Configure(root/"restart"));store.SetRemoveFault(true);store.SetWriteFault(2);auto a=store.Put(key,7,std::vector<uint8_t>(200,1));store.Drain();assert(a->state==WRITE_FAILED&&metric(store,"tempBytes")==172);observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
 }
 {ProgramArtifactStore store;assert(store.Configure(root/"restart"));assert(metric(store,"tempBytes")==0&&!store.Get(key));observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;}
 {
  ProgramArtifactStore store;assert(store.Configure(root/"queue"));auto old=store.Put(key,7,{1,2,3});store.Drain();std::promise<void> entered,release;auto future=release.get_future().share();store.Enqueue([&]{entered.set_value();future.wait();});entered.get_future().wait();
  std::promise<void> cleared;for(unsigned i=0;i<128;++i)store.Enqueue([&,i]{if(i==127)cleared.set_value();});auto failed=store.Put(next,7,{4,5});assert(failed&&failed->state==WRITE_FAILED&&store.Get(key)==old);assert(metric(store,"queueRejected")>=1);release.set_value();cleared.get_future().wait();store.Drain();assert(old->state==PERSISTED);observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
 }
 {
  ProgramArtifactStore store;assert(store.Configure(root/"pinned"));std::vector<std::shared_ptr<Artifact>> pinned;for(unsigned i=0;i<4;++i){pinned.push_back(store.Put(Hash("pin"+std::to_string(i)),7,std::vector<uint8_t>(kEntryLimit,1)));store.Drain();}
  assert(metric(store,"payloadSize")==kTotalLimit);assert(!store.Put(next,7,{4})&&metric(store,"pinRejected")>0);observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
  std::vector<std::thread> threads;for(unsigned i=0;i<8;++i)threads.emplace_back([&]{for(unsigned j=0;j<16;++j)assert(store.Get(pinned[0]->key)==pinned[0]);});for(auto& t:threads)t.join();while(metric(store,"queueJobs")==128)std::this_thread::yield();store.Drain();pinned.clear();assert(store.Put(next,7,{4}));store.Drain();observe(store);++passed;std::cout<<"PASS "<<passed<<std::endl;
 }
 std::cout<<"{\"revision\":\"T502_STORE_FAULTS_V1\",\"passed\":"<<passed<<",\"failed\":0,\"device\":\"NOT_RUN\"}"<<std::endl;
}
