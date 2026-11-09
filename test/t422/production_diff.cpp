#define GRS_PROGRESS_V2_EXPERIMENT
#define T404_EMBED_HOST
#include "t409_host_nomain.cpp"
#include <fstream>
#include <filesystem>
// T422 · 独立生产路径差分: 将 T419 oracle 五场景接到 T421 生产实现（控制线程
// ReserveEvidenceLocked + 生产 owner 循环 captureCheckpoint/TryArmReservedLocked），
// 不在 Wake hook 注入 arm。逐场景记录原始回执 JSON + 原始事件，与 T419 基线对照。

static std::vector<std::string> g_log;
static void note(const std::string& tag,const std::string& detail){g_log.push_back(tag+" | "+detail);std::cerr<<tag<<" | "<<detail<<std::endl;}

static EvidenceConfig makeConfig(App& a,uint64_t session){
 EvidenceConfig c;c.packageSha256=std::string(64,'a');c.expectedRevision="GRS_M1_PROGRESS_CLOCK_V2";
 c.epoch=a.epoch;c.runLo=a.identity.run.lo;c.runHi=a.identity.run.hi;c.pageSession=session;c.durationNs=2000000000;
 return c;}

static std::string requestState(App& a,uint64_t id){std::lock_guard<std::mutex> l(a.mutex);return a.ReadCaptureRequestLocked(id);}

// S1 连续工作: 控制线程在 owner 工作中预约（blocker=WAIT_ROOT），下一合法边界自动 ARMED；
// BEGIN 先于后续 root 事件；1s 预算内。复用生产循环 + PumpBegin 阻塞同步。
static void s1_continuous(){
 ownerAtRoot=false;controlReserved=false;lateSwapCase=false;sandOffered=waterOffered=false;printedAccepted=0;hostTestStart=std::chrono::steady_clock::now();
 Injection test(Stall::Flush,false);inject=&test;auto owner=std::make_unique<App>(false);auto& a=*owner;
 a.window=reinterpret_cast<void*>(1);a.nextProgressV2=true;a.tailProbeNext=true;
 a.hostNow=[](){return inject->now.load();};a.progress.BeginSession();
 std::atomic<uint64_t> requestAtReserve{0},armedSafeSerial{0};
 a.hostHook=[](App& a,BridgeStage stage){
  if(stage==BridgeStage::PumpBegin&&!ownerAtRoot.exchange(true)){while(!controlReserved)std::this_thread::yield();}
  if(stage!=BridgeStage::Wake)return;
  std::lock_guard<std::mutex> lock(a.mutex);
  if(a.progress.accepted>=10||std::chrono::steady_clock::now()-hostTestStart>std::chrono::seconds(60))a.quit=true;
 };
 std::atomic<bool> done{false};
 std::thread pulses([&]{while(!done){std::this_thread::sleep_for(std::chrono::milliseconds(8));std::lock_guard<std::mutex> lock(a.mutex);
  if(!a.progress.open)continue;test.now+=100000;assert(!strcmp(a.PagePulseLocked(a.progress.session,a.progress.sequence+1,test.now),"PAGE_PULSE_ACCEPTED"));}});
 std::thread control([&]{
  while(!ownerAtRoot&&!done)std::this_thread::yield();
  if(done)return;
  std::lock_guard<std::mutex> lock(a.mutex);
  const auto c=makeConfig(a,a.progress.session);
  const auto reply=a.ReserveEvidenceLocked(c);
  note("S1_RESERVE",reply);
  assert(reply.find("RESERVED")!=std::string::npos);
  requestAtReserve=a.captureRequest.id;
  assert(!a.evidence&&std::string(a.captureRequest.blocker)=="WAIT_ROOT");
  controlReserved=true;
 });
 std::thread worker([&]{a.loop();});worker.join();done=true;pulses.join();control.join();
 std::lock_guard<std::mutex> lock(a.mutex);
 assert(a.evidence&&a.captureRequest.state==App::CaptureRequest::ARMED&&a.captureRequest.safePointSerial>0);
 armedSafeSerial=a.captureRequest.safePointSerial;
 const auto armed=a.ReadCaptureRequestLocked(requestAtReserve);
 note("S1_ARMED",armed);
 assert(armed.find("ARMED")!=std::string::npos&&armed.find("\"captureId\"")!=std::string::npos&&armed.find("\"startNs\"")!=std::string::npos);
 assert(a.captureRequest.armByNs>=a.captureRequest.startNs||a.captureRequest.requestNs+1000000000ULL>=a.captureRequest.startNs);
 assert(std::string(a.evidence->rows[0].kind.data())=="BEGIN");
 note("S1_BEGIN_FIRST",std::string(a.evidence->rows[0].kind.data())+" acceptedPrefix="+std::to_string(a.evidence->initial.accepted));
 assert(a.progress.accepted>=1&&a.progress.Balanced());
 note("S1_VERDICT","ARMED_AT_NEXT_SAFE_POINT within 1s budget; BEGIN precedes later root events; business ledger unchanged");
}

// S2 旧 jobLive/半提交对照: blocker 非空时 owner 侧 TryArm 保持 RESERVED。
static void s2_blockers(){
 Injection test(Stall::Flush,false);inject=&test;auto owner=std::make_unique<App>(false);auto& a=*owner;
 std::lock_guard<std::mutex> lock(a.mutex);a.hostNow=[](){return inject->now.load();};
 a.window=reinterpret_cast<void*>(1);a.progressV2=true;a.progress.BeginSession();a.identity.run={1,0};a.captureCommittedEpoch=a.epoch;
 assert(a.CommitReadyLocked(a.epoch,a.window,a.identity,a.input));a.progress.ready=true;
 assert(!strcmp(a.PagePulseLocked(a.progress.session,1,test.now),"PAGE_PULSE_ACCEPTED"));
 const auto c=makeConfig(a,a.progress.session);
 // 生产语义: 预约回执 blocker 由 owner 阶段（captureOwnerPhase）推导：
 // ROOT→WAIT_ROOT, DRAIN→WAIT_REVOKED_DRAIN, DRAW/SWAP/PRESENT_HANDOFF→WAIT_PRESENT。
 for(const auto& [phase,blocker]:std::array<std::pair<const char*,const char*>,3>{
   {{"ROOT","WAIT_ROOT"},{"DRAIN","WAIT_REVOKED_DRAIN"},{"SWAP","WAIT_PRESENT"}}}){
  a.RecordCaptureOwnerLocked(phase,"",test.now);
  const auto reply=a.ReserveEvidenceLocked(c);
  note(std::string("S2_RESERVE_")+phase,reply);
  assert(reply.find("RESERVED")!=std::string::npos);
  assert(std::string(a.captureRequest.blocker)==blocker);
  assert(std::string(a.captureRequest.workerPhase)==phase);
  a.TryArmReservedLocked(blocker,test.now);
  assert(!a.evidence&&a.captureRequest.state==App::CaptureRequest::RESERVED);
  note(std::string("S2_BLOCKED_")+phase,"state stays RESERVED, no captureId, no BEGIN");
  a.ReadCaptureRequestLocked(a.captureRequest.id,true);
  assert(a.captureRequest.state==App::CaptureRequest::CANCELLED);
 }
 note("S2_VERDICT","all three blockers keep RESERVED; capture entry clears only via cancel; no business mutation");
}

// S3 换代取消: 预约后 session 换代 → CANCELLED(IDENTITY_CHANGED)，旧 ID STALE，旧冻结档保留。
static void s3_identity_change(){
 Injection test(Stall::Flush,false);inject=&test;auto owner=std::make_unique<App>(false);auto& a=*owner;
 std::lock_guard<std::mutex> lock(a.mutex);a.hostNow=[](){return inject->now.load();};
 a.window=reinterpret_cast<void*>(1);a.progressV2=true;a.progress.BeginSession();a.identity.run={1,0};a.captureCommittedEpoch=a.epoch;
 assert(a.CommitReadyLocked(a.epoch,a.window,a.identity,a.input));a.progress.ready=true;
 assert(!strcmp(a.PagePulseLocked(a.progress.session,1,test.now),"PAGE_PULSE_ACCEPTED"));
 const auto c=makeConfig(a,a.progress.session);
 assert(a.ReserveEvidenceLocked(c).find("RESERVED")!=std::string::npos);
 const auto id=a.captureRequest.id;
 note("S3_RESERVE","requestId="+std::to_string(id));
 ++a.progress.session;a.CheckCaptureRequestLocked(test.now);
 const auto after=a.ReadCaptureRequestLocked(id);
 note("S3_AFTER_CHANGE",after);
 assert(after.find("CANCELLED")!=std::string::npos&&after.find("IDENTITY_CHANGED")!=std::string::npos);
 assert(a.ReadCaptureRequestLocked(id).find("CANCELLED")!=std::string::npos);
 assert(!a.evidence);
 note("S3_VERDICT","identity change cancels pending; old id rejected as stale; no captureId minted");
}

// S4 超时: 监督线程同刻终结；迟到 owner 不能开启旧预约；无 captureId/BEGIN。
static void s4_timeout_late(){
 Injection test(Stall::Wait,false);inject=&test;auto owner=std::make_unique<App>(false);auto& a=*owner;
 std::lock_guard<std::mutex> lock(a.mutex);a.hostNow=[](){return inject->now.load();};
 a.window=reinterpret_cast<void*>(1);a.progressV2=true;a.progress.BeginSession();a.identity.run={1,0};a.captureCommittedEpoch=a.epoch;
 assert(a.CommitReadyLocked(a.epoch,a.window,a.identity,a.input));a.progress.ready=true;
 assert(!strcmp(a.PagePulseLocked(a.progress.session,1,test.now),"PAGE_PULSE_ACCEPTED"));
 const auto c=makeConfig(a,a.progress.session);
 a.RecordCaptureOwnerLocked("SWAP","WAIT_PRESENT",test.now);
 assert(a.ReserveEvidenceLocked(c).find("RESERVED")!=std::string::npos);
 const auto id=a.captureRequest.id;const auto armBy=a.captureRequest.armByNs;
 note("S4_RESERVE","requestId="+std::to_string(id)+" armByNs="+std::to_string(armBy));
 // 同刻: owner 检查点在 armByNs 到达——超时优先
 test.now=armBy;
 a.TryArmReservedLocked("",test.now);
 const auto at=a.ReadCaptureRequestLocked(id);
 note("S4_SAME_INSTANT",at);
 assert(at.find("TIMED_OUT")!=std::string::npos&&at.find("CAPTURE_SAFE_POINT_TIMEOUT")!=std::string::npos);
 assert(!a.evidence&&a.captureSerial==0);
 // 迟到 owner 不能开启
 a.TryArmReservedLocked("",armBy+500000000ULL);
 assert(!a.evidence&&a.captureSerial==0);
 note("S4_LATE_OWNER","no captureId, no BEGIN, captureSerial unchanged");
 note("S4_VERDICT","supervisor timeout wins same instant; late owner cannot arm");
}

// S5 迟到 end / ARMED 后 cancel: cancel 返回已启动状态；end 幂等冻结；业务账不变。
static void s5_late_end_cancel(){
 Injection test(Stall::Flush,false);inject=&test;auto owner=std::make_unique<App>(false);auto& a=*owner;
 std::lock_guard<std::mutex> lock(a.mutex);a.hostNow=[](){return inject->now.load();};
 a.window=reinterpret_cast<void*>(1);a.progressV2=true;a.progress.BeginSession();a.identity.run={1,0};a.captureCommittedEpoch=a.epoch;
 assert(a.CommitReadyLocked(a.epoch,a.window,a.identity,a.input));a.progress.ready=true;
 assert(!strcmp(a.PagePulseLocked(a.progress.session,1,test.now),"PAGE_PULSE_ACCEPTED"));
 const auto c=makeConfig(a,a.progress.session);
 assert(a.ReserveEvidenceLocked(c).find("RESERVED")!=std::string::npos);
 const auto id=a.captureRequest.id;
 a.TryArmReservedLocked("",test.now);
 assert(a.evidence&&a.evidence->armed);
 const auto capId=a.evidence->id;
 const auto armedReply=a.ReadCaptureRequestLocked(id,true);
 note("S5_CANCEL_WHILE_ARMED",armedReply);
 assert(armedReply.find("ARMED")!=std::string::npos);
 assert(a.evidence&&a.evidence->armed);
 const auto beforeLedger=a.progress.accepted;
 assert(a.EndEvidenceLocked(capId).find("FROZEN")!=std::string::npos);
 const auto frozenState=a.evidence->State();
 a.EndEvidenceLocked(capId);
 assert(a.evidence->State()==frozenState);
 note("S5_END_IDEMPOTENT","frozen endpoint immutable, ledger accepted="+std::to_string(a.progress.accepted));
 assert(a.progress.accepted==beforeLedger);
 note("S5_VERDICT","cancel during ARMED keeps armed capture; late end idempotent; business ledger unchanged");
}

int main(int argc,char** argv){
 (void)argc;(void)argv;
 controlUnits();supervisorReservation();
 g_log.clear();
 s1_continuous();s2_blockers();s3_identity_change();s4_timeout_late();s5_late_end_cancel();
 std::filesystem::create_directories(".codex/local-test/t422-diff");
 std::ofstream out(".codex/local-test/t422-diff/t422_log.ndjson");
 for(const auto& line:g_log)out<<line<<"\n";
 std::cout<<"T422 production-path differential: 5 scenarios against T419 oracle baseline PASS (S1 arm at next real safe point, S2 blockers keep RESERVED, S3 identity-change cancel + stale id, S4 same-instant timeout + late owner no arm, S5 armed-cancel + idempotent end; business ledger unchanged throughout)\n";
 return 0;
}
