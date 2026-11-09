#define GRS_PROGRESS_V2_EXPERIMENT
#define T404_EMBED_HOST
#include "../t398/app_host.cpp"
#include <fstream>
#include <filesystem>
void controlUnits(){
 Injection test(Stall::Flush,false);inject=&test;auto owner=std::make_unique<App>(false);auto& a=*owner;
 std::lock_guard<std::mutex> lock(a.mutex);a.hostNow=[](){return inject->now.load();};
 a.window=reinterpret_cast<void*>(1);a.progressV2=true;a.progress.BeginSession();
 a.identity.run={1,0};assert(a.CommitReadyLocked(a.epoch,a.window,a.identity,a.input));
 a.progress.ready=true;assert(!strcmp(a.PagePulseLocked(a.progress.session,1,test.now),"PAGE_PULSE_ACCEPTED"));
 assert(a.SetSourceLocked(-1));const auto boundary=a.ui.boundary,revision=a.permissionRevision;
 for(int i=0;i<10000;i++)assert(a.SetSourceLocked(-1));
 assert(a.ui.boundary==boundary&&a.permissionRevision==revision);
 a.count=1;const auto cancelled=a.cancelled;assert(a.SetSourceLocked(-1)&&a.count==0&&a.cancelled==cancelled+1);
 EvidenceConfig c;c.packageSha256=std::string(64,'a');c.expectedRevision="GRS_M1_PROGRESS_CLOCK_V2";
 c.epoch=a.epoch;c.runLo=1;c.pageSession=a.progress.session;c.durationNs=2000000000;
 a.captureCommittedEpoch=a.epoch;
 assert(a.ReserveEvidenceLocked(c).find("RESERVED")!=std::string::npos);
 const auto request=a.captureRequest.id,armBy=a.captureRequest.armByNs;
 assert(a.ReserveEvidenceLocked(c).find("RESERVED")!=std::string::npos&&a.captureRequest.id==request&&a.captureRequest.armByNs==armBy);
 for(const char* blocker:{"WAIT_ROOT","WAIT_REVOKED_DRAIN","WAIT_PRESENT"}){
  a.TryArmReservedLocked(blocker,test.now);assert(!a.evidence&&a.captureRequest.state==App::CaptureRequest::RESERVED);
 }
 a.TryArmReservedLocked("",test.now);assert(a.evidence&&a.evidence->armed);const auto id=a.evidence->id;
 assert(a.ReserveEvidenceLocked(c).find("CAPTURE_ALREADY_ARMED")!=std::string::npos);
 assert(a.ReadCaptureRequestLocked(request,true).find("ARMED")!=std::string::npos);
 assert(a.EndEvidenceLocked(id+1).find("STALE_CAPTURE_ID")!=std::string::npos);
 test.now+=16000000;assert(!strcmp(a.PagePulseLocked(a.progress.session,2,test.now),"PAGE_PULSE_ACCEPTED"));
 m1::PlanGrantV2 grant;assert(a.progress.Claim(test.now,0,a.ui.boundary,false,grant));
 m1::RootIsolation gate;
 // An unissued SOURCE BUSY intent may leave the numeric nonce unconsumed.
 // STOP then selects a QUANTUM: same nonce, distinct kind and intent deadline.
 a.FreezePermitLocked(a.epoch,a.window,gate,1,1);const auto sourceDeadline=a.permit.deadline;
 assert(a.SetSourceLocked(0));assert(a.SetSourceLocked(-1));test.now++;
 a.FreezePermitLocked(a.epoch,a.window,gate,1,2);assert(a.permit.deadline==sourceDeadline+1&&a.permitKind==2);
 RootRequest quantum;quantum.kind=2;quantum.nonce={1,0};assert(a.PermitLocked(quantum,test.now));
 AcceptedRoot result;result.nonce={1,0};result.physics={1,0};result.publicationGeneration={1,0};result.completion[27]=2;
 a.AcceptLocked(result);assert(a.progress.accepted==1&&a.progress.Balanced());
 test.now+=33333334;a.ObserveControlLocked(test.now);
 assert(a.evidence->frozen&&a.evidence->faultSeen&&a.evidence->revokeSeen&&a.progress.Balanced());
 assert(a.evidence->terminal.revoked==a.progress.revoked&&a.evidence->terminal.pending==0&&a.evidence->terminal.activeRemaining==0);
 const auto count=a.timeline.count;const auto frozen=a.evidence->State();
 for(unsigned i=0;i<10000;i++)a.PagePulseLocked(a.progress.session,3,test.now);
 assert(a.timeline.count==count&&frozen==a.EndEvidenceLocked(id));
 RootRequest oldSource;oldSource.kind=1;oldSource.nonce={1,0};assert(!a.PermitLocked(oldSource,test.now));
 assert(!a.progress.open&&std::string(a.evidence->rows[a.evidence->count-1].kind.data())=="END");
 // New capture identity cannot consume or replace any physical quota.
 c.pageSession++;
 assert(a.ReserveEvidenceLocked(c).find("IDENTITY_MISMATCH")!=std::string::npos);
 c.pageSession--;a.capturePrepared=App::NewCaptureStorage();
 assert(a.ReserveEvidenceLocked(c).find("RESERVED")!=std::string::npos);
 const auto timeout=a.captureRequest.id;assert(a.ReadCaptureRequestLocked(request).find("STALE_CAPTURE_REQUEST")!=std::string::npos);
 a.TryArmReservedLocked("",a.captureRequest.armByNs);
 assert(a.ReadCaptureRequestLocked(timeout).find("TIMED_OUT")!=std::string::npos&&a.evidence->State()==frozen);
 assert(a.ReserveEvidenceLocked(c).find("RESERVED")!=std::string::npos);
 const auto cancelledRequest=a.captureRequest.id;
 assert(a.ReadCaptureRequestLocked(cancelledRequest,true).find("CANCELLED")!=std::string::npos);
 a.TryArmReservedLocked("",test.now);assert(a.evidence->State()==frozen);
 assert(a.ReserveEvidenceLocked(c).find("RESERVED")!=std::string::npos);
 ++a.progress.session;a.CheckCaptureRequestLocked(test.now);
 assert(a.ReadCaptureRequestLocked(a.captureRequest.id).find("IDENTITY_CHANGED")!=std::string::npos&&a.evidence->State()==frozen);
 {
  auto second=std::make_unique<App>(false);auto& b=*second;std::lock_guard<std::mutex> secondLock(b.mutex);
  b.hostNow=a.hostNow;b.progressV2=true;b.window=reinterpret_cast<void*>(1);b.progress.BeginSession();
  assert(b.CommitReadyLocked(b.epoch,b.window,b.identity,b.input));b.progress.ready=true;
  assert(!strcmp(b.PagePulseLocked(b.progress.session,1,test.now),"PAGE_PULSE_ACCEPTED"));test.now+=16000000;
  assert(!strcmp(b.PagePulseLocked(b.progress.session,2,test.now),"PAGE_PULSE_ACCEPTED"));
  m1::PlanGrantV2 next;assert(b.progress.Claim(test.now,0,b.ui.boundary,true,next));m1::RootIsolation g;
  b.FreezePermitLocked(b.epoch,b.window,g,1,1);const auto sourceEnd=b.permit.deadline;
  b.SetSourceLocked(0);b.SetSourceLocked(-1);test.now++;
  b.FreezePermitLocked(b.epoch,b.window,g,1,2);assert(b.permit.deadline==sourceEnd+1);
  RootRequest old;old.kind=1;old.nonce={1,0};assert(!b.PermitLocked(old,test.now)&&b.AllowedLocked());
  RootRequest q;q.kind=2;q.nonce={1,0};assert(b.PermitLocked(q,test.now));
  b.AcceptLocked(result);assert(b.progress.accepted==1&&b.progress.Balanced());
 }
 std::cout<<"T409 actual App idempotent close/first revoke/one-shot fault/atomic terminal ledger/busy/stale capture PASS\n";
}
bool sandOffered=false,waterOffered=false;uint64_t printedAccepted=0;
auto hostTestStart=std::chrono::steady_clock::now();bool lateSwapCase=false;std::atomic<bool> ownerAtRoot{false},controlReserved{false};
void actualCadence(bool late=false){
 ownerAtRoot=false;controlReserved=false;lateSwapCase=late;sandOffered=waterOffered=false;printedAccepted=0;hostTestStart=std::chrono::steady_clock::now();
 Injection test(Stall::Flush,false);inject=&test;auto owner=std::make_unique<App>(false);auto& a=*owner;
 a.window=reinterpret_cast<void*>(1);a.nextProgressV2=true;a.tailProbeNext=true;
 a.hostNow=[](){return inject->now.load();};a.progress.BeginSession();
 a.hostHook=[](App& a,BridgeStage stage){
  if(stage==BridgeStage::PumpBegin&&!ownerAtRoot.exchange(true)){
   while(!controlReserved)std::this_thread::yield();
  }
  if(lateSwapCase&&stage==BridgeStage::SwapEnd){
   std::lock_guard<std::mutex> lock(a.mutex);if(!a.evidence)return;const auto target=a.permit.deadline+1;
   while(inject->now<target){inject->now+=std::min<uint64_t>(16000000,target-inject->now.load());
    assert(!strcmp(a.PagePulseLocked(a.progress.session,a.progress.sequence+1,inject->now),"PAGE_PULSE_ACCEPTED"));}
   return;
  }
  if(stage!=BridgeStage::Wake)return;
  std::lock_guard<std::mutex> lock(a.mutex);
  if(a.accepted!=printedAccepted){printedAccepted=a.accepted;std::cerr<<"ACCEPT n="<<a.accepted<<" q="<<a.progress.accepted<<" source="<<a.sourceCommit<<" drawn="<<a.renderCadence.newPhysicalFrames<<" phase="<<a.permitSourcePhase<<" status="<<a.phase<<std::endl;}
  if(a.progress.accepted>=20||std::chrono::steady_clock::now()-hostTestStart>std::chrono::seconds(120))a.quit=true;
  auto offer=[&](uint32_t phase,uint32_t seq){
   std::array<uint32_t,32> w{};w[0]=1129534023;w[1]=2;w[2]=phase;w[4]=a.identity.run.lo;w[5]=a.identity.run.hi;
   w[6]=w[15]=w[24]=seq;w[8]=a.input.lo;w[9]=a.input.hi;w[10]=1;w[12]=a.identity.geometry.lo;w[13]=a.identity.geometry.hi;
   w[14]=phase+1;w[27]=1;w[28]=a.identity.config.lo;w[29]=a.identity.config.hi;
   double x=64.5,y=phase?128.:0.,m=.125;std::memcpy(w.data()+16,&x,8);std::memcpy(w.data()+18,&y,8);std::memcpy(w.data()+20,&m,8);
   const auto result=a.OfferLocked(w);assert(!strcmp(result,"ENQUEUED_NOT_ACK"));a.CaptureOfferLocked(w,result);
  };
  if(!lateSwapCase&&a.evidence&&a.evidence->armed&&a.AllowedLocked()&&!sandOffered&&a.progress.accepted>=1){offer(0,1);sandOffered=true;}
  if(!lateSwapCase&&a.evidence&&a.evidence->armed&&a.AllowedLocked()&&!waterOffered&&a.sourceCommit==1&&a.renderCadence.newPhysicalFrames>=2){offer(1,2);waterOffered=true;}
  if((a.sourceCommit==2&&a.progress.accepted>=8&&a.renderCadence.newPhysicalFrames>=3)||a.status.find("\"FAULT\"")!=std::string::npos||std::string(a.phase)=="CLOCK_GAP"||std::string(a.phase)=="GPU_PROGRESS_TIMEOUT")a.quit=true;
 };
 std::atomic<bool> done{false};
 std::thread pulses([&]{while(!done){std::this_thread::sleep_for(std::chrono::milliseconds(8));std::lock_guard<std::mutex> lock(a.mutex);
  if(!a.progress.open)continue;test.now+=100000;const auto code=a.PagePulseLocked(a.progress.session,a.progress.sequence+1,test.now);assert(!strcmp(code,"PAGE_PULSE_ACCEPTED"));}});
 std::thread control([&]{
  while(!ownerAtRoot&&!done)std::this_thread::yield();
  if(done)return;
  std::lock_guard<std::mutex> lock(a.mutex);
  EvidenceConfig c;c.packageSha256=std::string(64,'a');c.expectedRevision="GRS_M1_PROGRESS_CLOCK_V2";
  c.epoch=a.epoch;c.runLo=a.identity.run.lo;c.runHi=a.identity.run.hi;c.pageSession=a.progress.session;c.durationNs=2000000000;
  assert(a.ReserveEvidenceLocked(c).find("RESERVED")!=std::string::npos);
  assert(!a.evidence&&std::string(a.captureRequest.blocker)=="WAIT_ROOT");controlReserved=true;
 });
 std::thread worker([&]{a.loop();});worker.join();done=true;pulses.join();control.join();
 std::lock_guard<std::mutex> lock(a.mutex);
 std::cerr<<"T409 ACTUAL accepted="<<a.accepted<<" q="<<a.progress.accepted<<" skipped="<<a.renderCadence.skipped<<" drawn="<<a.renderCadence.newPhysicalFrames<<" phase="<<a.phase<<" status="<<a.status<<std::endl;
 if(late){
  assert(a.accepted>=1&&a.progress.accepted==a.accepted&&a.progress.Balanced()&&a.lateSwaps>0);
  assert(a.evidence&&a.evidence->frozen&&!a.evidence->incomplete&&a.evidence->faultSeen&&a.evidence->revokeSeen);
  bool actualSwap=false;for(uint32_t i=0;i<a.evidence->count;i++){const auto& e=a.evidence->rows[i];
   if(std::string(e.kind.data())=="FIRST_FAILURE")actualSwap=e.control.swapReturned&&e.control.nativeSwapSucceeded&&e.control.observedReceiveNs>e.control.deadline;}
  const auto token=a.evidence->TokenIndex(a.permit.nonce);
  assert(actualSwap&&token<8&&a.evidence->tokens[token].requested[0]&&!a.evidence->tokens[token].presented);
  std::filesystem::create_directories(".codex/local-test/t409-late-pages");
  for(uint32_t i=0;i<a.evidence->PageCount();i++)std::ofstream(".codex/local-test/t409-late-pages/page"+std::to_string(i)+".json")<<a.evidence->Page(i);
  std::cout<<"T409 actual GLES Swap success then deadline+1ns; healthy scripted pulses; CURRENT rejected; requested token/lease/accepted prefix/frozen fault evidence PASS\n";
  return;
 }
 assert(a.progress.accepted>=8&&a.progress.accepted+2==a.accepted&&a.sourceCommit==2&&a.progress.Balanced());
 std::cerr<<" captureRows="<<(a.evidence?a.evidence->count:0)<<" captureDropped="<<(a.evidence?a.evidence->dropped:0)<<" captureReason="<<(a.evidence?a.evidence->reason.data():"NONE")<<std::endl;
 assert(a.renderCadence.skipped>0&&a.renderCadence.newPhysicalFrames<a.accepted);
 assert(a.evidence&&a.captureRequest.safePointSerial>0&&a.evidence->initial.accepted<=a.progress.accepted);
 assert(std::string(a.evidence->rows[0].kind.data())=="BEGIN");bool sand=false,water=false,sandPresent=false,waterPresent=false;
 for(uint32_t i=0;i<a.evidence->tokenCount;i++){const auto& t=a.evidence->tokens[i];
  sand|=t.kind==1&&t.phase==0&&t.admittedQ>0;water|=t.kind==1&&t.phase==1&&t.admittedQ>0;
  sandPresent|=t.kind==2&&t.phase==0&&t.presented&&t.swap;waterPresent|=t.kind==2&&t.phase==1&&t.presented&&t.swap;}
 assert(sand&&water&&sandPresent&&waterPresent&&a.evidence->tokenCount<=8);
 a.EndEvidenceLocked(a.evidence->id);assert(a.evidence->frozen&&!a.evidence->incomplete);
 std::filesystem::create_directories(".codex/local-test/t409-actual-pages");
 for(uint32_t i=0;i<a.evidence->PageCount();i++)std::ofstream(".codex/local-test/t409-actual-pages/page"+std::to_string(i)+".json")<<a.evidence->Page(i);
 std::cout<<"T409 actual ANGLE App 30Hz skips Draw only; sand+water SOURCE and >=8 accepted QUANTUM, phase PRESENT tokens <=8, fixed capture counters/first-root details PASS\n";
}
void supervisorReservation(){
 Injection test(Stall::Wait,false);inject=&test;auto owner=std::make_unique<App>(false);auto& a=*owner;
 uint64_t request=0;
 {
  std::lock_guard<std::mutex> lock(a.mutex);a.hostNow=[](){return inject->now.load();};
  a.window=reinterpret_cast<void*>(1);a.progress.BeginSession();a.identity.run={1,0};a.captureCommittedEpoch=a.epoch;
  EvidenceConfig c;c.packageSha256=std::string(64,'a');c.expectedRevision="GRS_M1_PROGRESS_CLOCK_V1";
  c.epoch=a.epoch;c.runLo=1;c.pageSession=a.progress.session;c.durationNs=2000000000;
  a.RecordCaptureOwnerLocked("SWAP","WAIT_PRESENT",test.now);
  assert(a.ReserveEvidenceLocked(c).find("RESERVED")!=std::string::npos);
  request=a.captureRequest.id;test.now=a.captureRequest.armByNs;
 }
 for(int i=0;i<100;i++){
  {std::lock_guard<std::mutex> lock(a.mutex);
   if(a.captureRequest.state==App::CaptureRequest::TIMED_OUT){
    assert(a.ReadCaptureRequestLocked(request).find("WAIT_PRESENT")!=std::string::npos);
    a.TryArmReservedLocked("",test.now);assert(!a.evidence&&a.captureSerial==0);
    std::cout<<"T421 independent supervisor timeout at equality while owner blocked; late checkpoint cannot arm PASS"<<std::endl;return;
   }}
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
 }
 assert(false);
}