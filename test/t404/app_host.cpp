#define GRS_PROGRESS_V2_EXPERIMENT
#define T404_EMBED_HOST
#include "../t398/app_host.cpp"

void v2Case(unsigned delay,bool stopPulse){
 const auto storage=std::make_unique<App>(false);auto& app=*storage;
 Injection test(Stall::Flush,true);inject=&test;
 app.window=reinterpret_cast<void*>(1);app.nextProgressV2=true;app.tailProbeNext=true;
 app.hostNow=[](){return inject->now.load();};
 app.progress.BeginSession();
 app.hostHook=[](App& a,BridgeStage stage){
  if(stage!=BridgeStage::Wake&&stage!=BridgeStage::PumpBegin)return;
  if(stage==BridgeStage::PumpBegin){
   std::lock_guard<std::mutex> lock(inject->mutex);inject->hot=true;return;
  }
  std::lock_guard<std::mutex> lock(a.mutex);
  if(a.status.find("\"CURRENT\"")!=std::string::npos||
     a.status.find("\"FAULT\"")!=std::string::npos||
     std::string(a.phase)=="GPU_PROGRESS_TIMEOUT"||std::string(a.phase)=="CLOCK_GAP"){
   std::cerr<<"TERMINAL "<<a.status<<" quota="<<a.progress.generated<<" active="<<a.progress.activeRemaining<<std::endl;
   a.quit=true;
  }
  if(!inject->offered&&a.AllowedLocked()){
   assert(std::string(putSource(a))=="ENQUEUED_NOT_ACK");inject->offered=true;
  }
 };
 std::atomic<bool> send{true},done{false};
 std::thread pulses([&]{
  while(!done){
   std::this_thread::sleep_for(std::chrono::milliseconds(8));
   if(!send)continue;
   std::lock_guard<std::mutex> lock(app.mutex);
   if(!app.progress.open)continue;
   test.now+=1000000;
   const auto seq=app.progress.sequence+1;
   const auto result=app.PagePulseLocked(app.progress.session,seq,test.now);
   if(strcmp(result,"PAGE_PULSE_ACCEPTED")){
    std::cerr<<"PULSE "<<result<<" session="<<app.progress.session<<" seq="<<seq
      <<" stored="<<app.progress.sequence<<" native="<<test.now<<" prev="<<app.progress.receiveNs<<std::endl;
    assert(false);
   }
  }
 });
 std::thread worker([&]{app.loop();});
 {
  std::unique_lock<std::mutex> lock(test.mutex);
  if(!test.cv.wait_for(lock,std::chrono::seconds(160),[&]{return test.entered;})){
   std::cerr<<"NO_ROOT status="<<app.status<<" ready="<<app.ready<<" permission="<<app.permission
    <<" pulseOpen="<<app.progress.open<<" pulseReady="<<app.progress.ready<<" fault="<<app.progress.fault
    <<" pending="<<app.progress.pending<<" inputOffered="<<test.offered<<std::endl;assert(false);
  }
 }
 if(stopPulse)send=false;
 if(!stopPulse){
  for(unsigned ms=0;ms<delay;ms+=16){
   std::this_thread::sleep_for(std::chrono::milliseconds(16));
   std::lock_guard<std::mutex> lock(app.mutex);
   test.now+=16000000;
   assert(!strcmp(app.PagePulseLocked(app.progress.session,app.progress.sequence+1,test.now),"PAGE_PULSE_ACCEPTED"));
  }
 }else std::this_thread::sleep_for(std::chrono::milliseconds(delay));
 {
  std::lock_guard<std::mutex> lock(app.mutex);
  if(stopPulse){
   test.now+=40000000;app.ObserveControlLocked(test.now);
   assert(!app.AllowedLocked()&&app.accepted==0&&app.progress.Balanced());app.quit=true;
  }else{
   assert(app.AllowedLocked()&&app.progress.Balanced());
   assert(app.progress.pending<=4&&app.progress.overload>0);
  }
 }
 {std::lock_guard<std::mutex> lock(test.mutex);test.release=true;test.cv.notify_one();}
 worker.join();done=true;pulses.join();
 if(stopPulse)assert(app.accepted==0);
 else assert(app.accepted>=1&&app.sourceCommit==1&&app.progress.accepted==0);
 assert(app.progress.Balanced());
 std::cout<<"T404 actual App V2 slow root "<<delay<<"ms stoppedPulse="<<stopPulse
  <<" accepted="<<app.accepted<<" generated="<<app.progress.generated<<" dropped="<<app.progress.overload<<" PASS\n";
}

void permitCases(){
 {
  auto storage=std::make_unique<App>(false);auto& a=*storage;
  Injection test(Stall::Flush,true);inject=&test;
  std::lock_guard<std::mutex> lock(a.mutex);a.hostNow=[](){return inject->now.load();};
  a.window=reinterpret_cast<void*>(1);assert(a.CommitReadyLocked(a.epoch,a.window,a.identity,a.input));
  const auto revision=a.permissionRevision;const auto rows=a.timeline.count;
  const auto session=a.BeginPageSessionLocked();
  assert(!strcmp(a.PagePulseLocked(session,1,test.now),"PAGE_PULSE_ACCEPTED"));
  assert(a.AllowedLocked()&&a.permissionRevision==revision&&a.timeline.count==rows);
 }

 for(const uint64_t duration:{5000000ULL,40000000ULL,65000000ULL,200000000ULL,250000000ULL,250000001ULL}){
  auto storage=std::make_unique<App>(false);auto& a=*storage;
  Injection test(Stall::Flush,true);inject=&test;
  std::lock_guard<std::mutex> lock(a.mutex);
  a.hostNow=[](){return inject->now.load();};a.progressV2=true;
  a.window=reinterpret_cast<void*>(1);a.progress.BeginSession();
  assert(a.CommitReadyLocked(a.epoch,a.window,a.identity,a.input));
  a.progress.ready=true;
  assert(!strcmp(a.PagePulseLocked(a.progress.session,1,test.now),"PAGE_PULSE_ACCEPTED"));
  m1::PlanGrantV2 grant;assert(a.progress.Claim(test.now,0,a.ui.boundary,true,grant));
  m1::RootIsolation gate;RootRequest root;root.kind=1;root.nonce={1,0};
  a.lastGoodControl="previous-accepted-token";
  a.FreezePermitLocked(a.epoch,a.window,gate,1,1);
  assert(a.status.find("STALE")!=std::string::npos);
  const auto start=test.now.load(),deadline=a.permit.deadline;
  // BUSY/continuation repeatedly presents the same intent; no renewal.
  while(test.now-start<duration){
   test.now+=std::min<uint64_t>(16000000,duration-(test.now-start));
   assert(!strcmp(a.PagePulseLocked(a.progress.session,a.progress.sequence+1,test.now),"PAGE_PULSE_ACCEPTED"));
   a.FreezePermitLocked(a.epoch,a.window,gate,1,1);
   assert(a.permit.deadline==deadline);
  }
  assert(a.PermitLocked(root,test.now)==(duration<=250000000));
  assert(a.progress.Balanced());
 }
 {
  auto storage=std::make_unique<App>(false);auto& a=*storage;
  Injection test(Stall::Flush,true);inject=&test;
  std::lock_guard<std::mutex> lock(a.mutex);
  a.hostNow=[](){return inject->now.load();};a.progressV2=true;
  a.window=reinterpret_cast<void*>(1);a.progress.BeginSession();
  assert(a.CommitReadyLocked(a.epoch,a.window,a.identity,a.input));a.progress.ready=true;
  assert(!strcmp(a.PagePulseLocked(a.progress.session,1,test.now),"PAGE_PULSE_ACCEPTED"));
  test.now+=16000000;assert(!strcmp(a.PagePulseLocked(a.progress.session,2,test.now),"PAGE_PULSE_ACCEPTED"));
  m1::PlanGrantV2 grant;assert(a.progress.Claim(test.now,0,a.ui.boundary,true,grant));
  m1::RootIsolation gate;RootRequest source;source.kind=1;source.nonce={1,0};
  a.FreezePermitLocked(a.epoch,a.window,gate,1,1);const auto sourceDeadline=a.permit.deadline;
  assert(a.SetSourceLocked(-1));assert(!a.PermitLocked(source,test.now));
  assert(a.AllowedLocked()&&a.progress.activeRemaining>0);
  test.now+=1000000;
  RootRequest quantum;quantum.kind=2;quantum.nonce={2,0};
  a.FreezePermitLocked(a.epoch,a.window,gate,2,2);
  assert(a.permit.deadline==sourceDeadline+1000000&&a.PermitLocked(quantum,test.now));
  assert(a.SetSourceLocked(-1)&&a.PermitLocked(quantum,test.now));
  AcceptedRoot accepted;accepted.completion[27]=2;accepted.nonce={2,0};
  a.AcceptLocked(accepted);assert(a.progress.accepted==1&&a.progress.Balanced());
  const auto pulseSequence=a.progress.sequence,received=a.progress.receiveNs;
  a.ObserveControlLocked(test.now);a.ui.Fresh(test.now);
  assert(a.progress.sequence==pulseSequence&&a.progress.receiveNs==received);
  const auto owner=a.identity.owner;a.identity.owner.lo++;
  assert(!a.PermitLocked(quantum,test.now));a.identity.owner=owner;
  // A late supervisor does not allow authorize to skip freshness.
  test.now+=33333334;assert(!a.PermitLocked(quantum,test.now));
  assert(!a.AllowedLocked()&&a.progress.Balanced());
 }
 std::cout<<"T404 actual permit 5/40/65/200/250ms/+1ns; BUSY fixed deadline; UP source-only; independent quantum; late supervisor GAP PASS\n";
}

int main(int argc,char** argv){
 permitCases();
 if(argc>1&&std::string(argv[1])=="--permit-only")return 0;
 v2Case(65,false);
 v2Case(65,true);
}
