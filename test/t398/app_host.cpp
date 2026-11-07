#define main t324_original_app_main
#include "../t324/app_gl.cpp"
#undef main

namespace {
enum class Stall {Flush,Wait,Map,BeforeAccept,BeforeSwap,AfterSwap};
struct Injection {
 Stall point;
 bool source;
 std::mutex mutex;
 std::condition_variable cv;
 bool hot=false,entered=false,release=false;
 std::atomic<uint64_t> now{1000000};
 bool offered=false;
 explicit Injection(Stall cut,bool isSource):point(cut),source(isSource){}
 void StallAt(Stall at){
  std::unique_lock<std::mutex> lock(mutex);
  if(!hot||entered||point!=at)return;
  entered=true;cv.notify_one();
  cv.wait(lock,[&]{return release;});
 }
};
Injection* inject=nullptr;
const char* putSource(App& app){
 const auto id=app.identity;
 std::array<uint32_t,32> w{};
 w[0]=1129534023;w[1]=2;w[2]=1;w[4]=id.run.lo;w[5]=id.run.hi;
 w[6]=w[8]=w[10]=w[14]=w[15]=w[24]=w[27]=1;
 w[12]=id.geometry.lo;w[13]=id.geometry.hi;w[28]=id.config.lo;w[29]=id.config.hi;
 double x=64.5,y=0,m=64;
 std::memcpy(w.data()+16,&x,8);std::memcpy(w.data()+18,&y,8);std::memcpy(w.data()+20,&m,8);
 return app.OfferLocked(w);
}
void workerHook(App& app,BridgeStage stage){
 auto& test=*inject;
 if(stage==BridgeStage::Wake){
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  std::lock_guard<std::mutex> lock(app.mutex);
  if(app.status.find("\"CURRENT\"")!=std::string::npos)app.quit=true;
  if(!test.offered&&app.AllowedLocked()){
   if(test.source)assert(std::string(putSource(app))=="ENQUEUED_NOT_ACK");
   test.offered=true;test.now+=8400000;
  }else test.now++;
 }
 if(stage==BridgeStage::PumpBegin&&test.offered){
  std::lock_guard<std::mutex> lock(test.mutex);
  test.hot=true;
 }
 if(stage==BridgeStage::RootPoll)test.StallAt(Stall::BeforeAccept);
 if(stage==BridgeStage::SwapBegin)test.StallAt(Stall::BeforeSwap);
 if(stage==BridgeStage::SwapEnd)test.StallAt(Stall::AfterSwap);
}
void runCase(Stall cut,int millis,bool source,bool invalidate,bool indefinite=false){
 const auto storage=std::make_unique<App>(false);
 auto& app=*storage;
 Injection test(cut,source);
 inject=&test;
 app.window=reinterpret_cast<void*>(1);
 app.tailProbeNext=true;
 app.hostNow=[](){return inject->now.load();};
 app.hostHook=workerHook;
 std::thread owner([&]{app.loop();});
 {
  std::unique_lock<std::mutex> lock(test.mutex);
  assert(test.cv.wait_for(lock,std::chrono::seconds(150),[&]{return test.entered;}));
 }
 const bool swap=cut==Stall::BeforeSwap||cut==Stall::AfterSwap;
 const bool pass=millis==5&&!invalidate&&!indefinite;
 if(millis>0)std::this_thread::sleep_for(std::chrono::milliseconds(millis));
 test.now+=uint64_t(millis)*1000000;
 {
  // Before releasing the driver stand-in, status and the full 16-slot
  // control mailbox can acquire the mutex and make independent progress.
  std::unique_lock<std::mutex> lock(app.mutex,std::try_to_lock);
  assert(lock.owns_lock());
  assert(app.accepted==(swap?1u:0u));
  app.ObserveControlLocked(app.Stamp());
  if(!pass&&!invalidate)assert(std::string(app.permission)=="PAUSED_CLOCK");
  if(pass)assert(app.AllowedLocked());
  if(indefinite){
   while(app.count<app.queue.size())assert(std::string(putSource(app))=="ENQUEUED_NOT_ACK");
   assert(std::string(putSource(app))=="BACKPRESSURE");
  }
 }
 if(invalidate){
  assert(app.setActive(false));
  std::lock_guard<std::mutex> lock(app.mutex);
  assert(!app.AllowedLocked()&&app.count==0);
 }
 if(indefinite)std::this_thread::sleep_for(std::chrono::milliseconds(100));
 if(!pass){
  std::lock_guard<std::mutex> lock(app.mutex);
  app.quit=true;
 }
 {
  std::lock_guard<std::mutex> lock(test.mutex);
  assert(!test.release);
  test.release=true;test.cv.notify_one();
 }
 owner.join();
 assert(app.accepted==((pass||swap)?1u:0u));
 if(pass){
  assert(app.status.find("\"CURRENT\"")!=std::string::npos);
  assert(app.completionCount==1&&app.physicsCommit==1&&app.sourceCommit==(source?1u:0u));
 }else{
  assert(app.status.find("\"CURRENT\"")==std::string::npos);
  if(!swap)assert(app.completionCount==0&&app.physicsCommit==0&&app.sourceCommit==0);
  if(cut==Stall::AfterSwap)assert(app.lateSwaps==1);
 }
 std::cout<<"T398 cut="<<int(cut)<<" ms="<<millis<<" source="<<source
  <<" indefinite="<<indefinite<<" accepted="<<app.accepted<<" status="<<app.status<<std::endl;
}
}
void T398HostGlPoint(int point){
 if(!inject)return;
 inject->StallAt(point==1?Stall::Flush:point==2?Stall::Wait:Stall::Map);
}
#ifndef T404_EMBED_HOST
int main(int argc,char**){
 {
  const auto storage=std::make_unique<App>(false);
  auto& app=*storage;
  app.window=reinterpret_cast<void*>(1);
  Identity id;id.run=id.owner=id.surface=id.context=id.geometry=id.config={1,0};
  assert(app.CommitReadyLocked(app.epoch,app.window,id,{1,0}));
  app.controlAnchor=100;
  m1::RootIsolation gate;
  app.FreezePermitLocked(app.epoch,app.window,gate);
  RootRequest root;root.nonce={1,0};
  assert(app.PermitLocked(root,100+App::kGapNs));
  assert(!app.PermitLocked(root,101+App::kGapNs));
  assert(std::string(app.permission)=="PAUSED_CLOCK");
  std::cout<<"T398 exact deadline equality/+1ns and queued control semantics PASS\n";
 }
 if(argc>1){
  runCase(Stall::Flush,0,true,true,true);
  return 0;
 }
 runCase(Stall::Flush,5,true,false);
 runCase(Stall::Flush,40,false,false);
 runCase(Stall::Flush,65,true,false);
 runCase(Stall::Flush,0,true,true,true);
 runCase(Stall::Wait,40,true,false);
 runCase(Stall::Map,40,false,false);
 runCase(Stall::BeforeAccept,40,true,false);
 runCase(Stall::BeforeSwap,0,true,true);
 runCase(Stall::AfterSwap,0,true,true);
 std::cout<<"T398 owner/control isolation 9/9\n";
}

#endif
