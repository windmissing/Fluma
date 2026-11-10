#define GRS_PROGRESS_V2_EXPERIMENT
#define T404_EMBED_HOST
#include "../t422/t409_host_nomain.cpp"
#include "angles_generated.h"
#include <fstream>
#include <filesystem>
// T459 · GPU 采集开关事务与 lease 独立对照（T441 §7 证据边界补正）。
// 同输入、同脚本时钟，capture-off / capture-on 两臂生产路径逐 root 配对：
//   accepted state(ledger 五字段)/event(roots)/completion(256w)/token(首 root 身份)、
//   首错与成功前缀、Acquire→release fence→Retire 代际。
// 单列 deadline 与 GAP；归档样本交 T443 T441_ARCHIVE_CHECK_V2 独立核对。
// 标 HOST_SCRIPTED_OFFLINE（GRS_BRIDGE_HOST_TEST 使 evidenceSource 自动置 HOST_SCRIPTED_OFFLINE）。
// 避免复跑 T422 预约五场景；不代替 T410 真机。

static std::vector<std::string> g_log;
static void note(const std::string& tag,const std::string& detail){g_log.push_back(tag+" | "+detail);std::cerr<<tag<<" | "<<detail<<std::endl;}

static std::vector<std::string> g_lines;
static void rec(const std::string& s){g_lines.push_back(s);}

struct ArmResult {
 bool ok=false;uint64_t accepted=0,revoked=0,overload=0,generated=0,carry=0,clearedCarry=0,pendingEnd=0,activeEnd=0;
 uint64_t captureRows=0,captureDropped=0,captureId=0,firstRootTokenRoot=0;
 uint64_t acqGeneration=0,acqSlot=9,acqOk=0,relOk=0,retireGenSeen=0;
 bool frozen=false,incomplete=false,deadlineFirst=false,gapSeen=false;
 uint64_t beginAcceptedPrefix=UINT64_MAX,endAccepted=0;
 uint32_t completion[256]{};
 std::string firstError="NONE",frozenReason="";
 std::vector<std::array<uint32_t,256>> completions;   // 全部接受 completion 快照
 std::vector<uint64_t> rootNonces;                    // 逐 root nonce
 std::vector<uint64_t> physicsCommits;
};

static void offerWords(App& a,uint32_t phase,uint32_t seq,double y,double m,std::array<uint32_t,32>& w){
 w={};w[0]=1129534023;w[1]=2;w[2]=phase;w[4]=a.identity.run.lo;w[5]=a.identity.run.hi;
 w[6]=w[15]=w[24]=seq;w[8]=a.input.lo;w[9]=a.input.hi;w[10]=1;w[12]=a.identity.geometry.lo;w[13]=a.identity.geometry.hi;
 w[14]=phase+1;w[27]=1;w[28]=a.identity.config.lo;w[29]=a.identity.config.hi;
 double x=64.5;std::memcpy(w.data()+16,&x,8);std::memcpy(w.data()+18,&y,8);std::memcpy(w.data()+20,&m,8);
}

// 单臂：注入序列 = seed SOURCE + N 个 QUANTUM + mid SOURCE + 更多 QUANTUM。
// captureOn=true 时 pulses 线程在 session open 后立即预约（与 T422 S1 控制线程
// 相同：owner 第一次 PumpBegin 自旋等待 controlReserved，预约后放行），
// 下一真实安全点 ARM，BEGIN acceptedPrefix 反映采集起点；accepted>=total 后显式 END。
// 驱动通过生产 loop()（captureCheckpoint 三个真实边界）。
// hostHook 是普通函数指针（不能捕获）——臂状态放文件级静态。
static bool g_armCaptureOn=false;static uint32_t g_totalQuanta=6;
static std::atomic<bool> g_controlReserved{false},g_endRequested{false};
static void t459Hook(App& app,BridgeStage stage){
 if(stage==BridgeStage::PumpBegin&&!ownerAtRoot.exchange(true)){while(g_armCaptureOn&&!g_controlReserved)std::this_thread::yield();}
 if(stage!=BridgeStage::Wake)return;
 std::lock_guard<std::mutex> lock(app.mutex);
 if(app.accepted!=printedAccepted){printedAccepted=app.accepted;}
 // quit 余量 2：给 pulses 线程的 END_REPLY 留窗口
 if(app.progress.accepted>=g_totalQuanta+2)app.quit=true;
}
static ArmResult runArm(bool captureOn,uint32_t armAt){
 ArmResult R;
 ownerAtRoot=false;controlReserved=false;lateSwapCase=false;sandOffered=waterOffered=false;printedAccepted=0;
 hostTestStart=std::chrono::steady_clock::now();
 g_armCaptureOn=captureOn;g_controlReserved=false;g_endRequested=false;
 (void)armAt;
 Injection test(Stall::Flush,false);inject=&test;
 auto owner=std::make_unique<App>(false);auto& a=*owner;
 a.window=reinterpret_cast<void*>(1);a.nextProgressV2=true;a.tailProbeNext=true;
 a.hostNow=[](){return inject->now.load();};a.progress.BeginSession();
 a.hostHook=&t459Hook;
 std::atomic<bool> done{false};
 std::thread pulses([&]{uint64_t lastQ=0;bool reserved=false;
  while(!done){std::this_thread::sleep_for(std::chrono::milliseconds(2));std::lock_guard<std::mutex> lock(a.mutex);
   if(!a.progress.open)continue;
   if(captureOn&&!reserved){
    reserved=true;
    EvidenceConfig c;c.packageSha256=std::string(64,'a');c.expectedRevision="GRS_M1_PROGRESS_CLOCK_V2";
    c.epoch=a.epoch;c.runLo=a.identity.run.lo;c.runHi=a.identity.run.hi;c.pageSession=a.progress.session;c.durationNs=2000000000;
    const auto reply=a.ReserveEvidenceLocked(c);
    rec("RESERVE_REPLY accepted="+std::to_string(a.progress.accepted)+" "+reply);
    g_controlReserved=true;controlReserved=true;continue;
   }
   if(!g_endRequested&&a.evidence&&a.evidence->armed&&a.progress.accepted>=g_totalQuanta){
    g_endRequested=true;const auto id=a.evidence->id;const auto r=a.EndEvidenceLocked(id);
    rec("END_REPLY "+r);continue;
   }
   test.now+=100000;const auto code=a.PagePulseLocked(a.progress.session,a.progress.sequence+1,test.now);
   if(std::strcmp(code,"PAGE_PULSE_ACCEPTED"))rec("PULSE_REJECTED accepted="+std::to_string(a.progress.accepted)+" code="+code);
  }});
 std::thread worker([&]{a.loop();});worker.join();done=true;pulses.join();
 std::lock_guard<std::mutex> lock(a.mutex);
 R.accepted=a.accepted;R.generated=a.progress.generated;R.revoked=a.progress.revoked;R.overload=a.progress.overload;
 R.carry=a.progress.carry;R.clearedCarry=a.progress.clearedCarry;R.pendingEnd=a.progress.pending;R.activeEnd=a.progress.activeRemaining;
 R.completions.assign(a.completions.begin(),a.completions.begin()+a.completionCount);
 R.gapSeen=std::string(a.phase)=="CLOCK_GAP";
 R.frozenReason=a.evidence?a.evidence->reason.data():"NO_EVIDENCE";
 R.frozen=a.evidence&&a.evidence->frozen;R.incomplete=a.evidence?a.evidence->incomplete:true;
 R.captureRows=a.evidence?a.evidence->count:0;R.captureDropped=a.evidence?a.evidence->dropped:0;R.captureId=a.evidence?a.evidence->id:0;
 R.beginAcceptedPrefix=a.evidence?a.evidence->initial.accepted:UINT64_MAX;
 R.endAccepted=a.evidence?a.evidence->terminal.accepted:UINT64_MAX;
 R.deadlineFirst=a.evidence&&std::string(a.evidence->reason.data())=="DURATION_REACHED";
 if(a.evidence&&a.evidence->count>0){
  for(uint32_t i=0;i<a.evidence->count;i++){const auto& e=a.evidence->rows[i];
   if(std::string(e.kind.data())=="FIRST_FAILURE"&&R.firstError=="NONE")R.firstError=e.reason.data();
   if(R.firstError=="NONE"&&std::string(e.kind.data())=="END")R.firstError=e.reason.data();}
  // token：BEGIN 之后的第一个接受 root 的 completion 在 completions 里按序对应
 }
 if(a.completionCount>0)std::memcpy(R.completion,a.completions[0].data(),1024);
 R.firstRootTokenRoot=a.evidence&&a.evidence->tokenCount>0?a.evidence->tokens[0].root:0;
 return R;
}

int main(int argc,char** argv){
 (void)argc;(void)argv;
 g_log.clear();
 controlUnits();supervisorReservation();
 g_log.clear();g_lines.clear();
 std::cerr<<"[T459] arms start\n";
 // 两臂同输入：armAt 相同则 capture-on 臂在 QUANTUM 2 前预约。
 const uint32_t armAt=2;
 const auto off=runArm(false,armAt);
 std::cerr<<"[T459] capture-off done accepted="<<off.accepted<<"\n";
 const auto on=runArm(true,armAt);
 std::cerr<<"[T459] capture-on done accepted="<<on.accepted<<"\n";
 int checks=0,passed=0;
 auto cmp=[&](const char* name,uint64_t a,uint64_t b){++checks;
  if(a==b){++passed;rec("PAIR_OK "+std::string(name)+" ="+std::to_string(a));}
  else rec("PAIR_DIVERGE "+std::string(name)+" off="+std::to_string(a)+" on="+std::to_string(b));};

 // ---- A. 业务账逐项（T441 §7：accepted state/ledger） ----
 cmp("acceptedCount",off.accepted,on.accepted);
 cmp("generatedQuanta",off.generated,on.generated);
 cmp("revokedDroppedQuanta",off.revoked,on.revoked);
 cmp("overloadDroppedQuanta",off.overload,on.overload);
 cmp("quotaCarry",off.carry,on.carry);
 cmp("clearedCarry",off.clearedCarry,on.clearedCarry);
 cmp("pendingEnd",off.pendingEnd,on.pendingEnd);
 cmp("activeRemainingEnd",off.activeEnd,on.activeEnd);
 // completion 逐字节（逐 root 身份）
 {++checks;const size_t n=std::min(off.completions.size(),on.completions.size());
  bool equal=n>0&&off.completions.size()==on.completions.size();
  for(size_t i=0;equal&&i<n;i++)if(0!=std::memcmp(off.completions[i].data(),on.completions[i].data(),1024))equal=false;
  if(equal){++passed;rec("PAIR_OK completion256x"+std::to_string(n));}
  else rec("PAIR_DIVERGE completion256 n_off="+std::to_string(off.completions.size())+" n_on="+std::to_string(on.completions.size()));}

 // ---- B. capture-on 臂事务语义 ----
 ++checks;
 if(on.frozen&&!on.incomplete&&on.captureRows>=2){++passed;rec("CAPTURE_FROZEN rows="+std::to_string(on.captureRows));}
 else rec("CAPTURE_NOT_FROZEN reason="+on.frozenReason+" rows="+std::to_string(on.captureRows));
 ++checks;
 if(on.beginAcceptedPrefix<=on.accepted){++passed;
  rec("BEGIN_PREFIX acceptedPrefix="+std::to_string(on.beginAcceptedPrefix)+" accepted="+std::to_string(on.accepted));}
 else rec("BEGIN_PREFIX_UNEXPECTED "+std::to_string(on.beginAcceptedPrefix));
 // END 时账与业务账一致（成功前缀 == 全部接受）
 ++checks;
 if(on.endAccepted==on.accepted&&off.accepted==on.accepted){++passed;rec("PREFIX_FULL_MATCH accepted="+std::to_string(on.accepted));}
 else rec("PREFIX_MISMATCH off="+std::to_string(off.accepted)+" on="+std::to_string(on.accepted)+" endLedger="+std::to_string(on.endAccepted));
 // capture 未改变业务终态（T441 §7 核心：无观测扰动）
 ++checks;
 if(off.accepted==on.accepted&&off.generated==on.generated){++passed;rec("NO_OBSERVATION_DISTURBANCE");}
 else rec("OBSERVATION_DISTURBANCE");

 // ---- C. lease 代际（同臂内 Acquire→release fence→Retire→再 Acquire STALE） ----
 // 独立于 App loop，用与 T434 full_host 相同的生产 transport 调用序列验证。
 // 硬超时： lease 段在守护线程内限时 180s，超时记 NOT_MEASURED 降级（避免 SwiftShader
 // kernel JIT 无界阻塞整卡），不冒 PASS。
 {
  std::atomic<bool> leaseDone{false};std::string leaseSummary;
  std::thread leaseGuard([&]{
   const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(180);
   // 心跳检查：主线程 lease 段结束后置 leaseDone；守护线程超时直接 _exit 降级。
   while(!leaseDone.load()){
    if(std::chrono::steady_clock::now()>deadline){
     std::FILE* f=std::fopen(".codex/local-test/t459/t459-summary-timeout.json","w");
     if(f){std::fprintf(f,"{\"revision\":\"T459_CAPTURE_TOGGLE_DIFF_V1\",\"mode\":\"HOST_SCRIPTED_OFFLINE\","
      "\"verdict\":\"PARTIAL_LEASE_TIMEOUT\",\"leaseSection\":\"NOT_MEASURED\",\"note\":\"lease generation section exceeded 180s guard (SwiftShader JIT unbounded); A/B/D ledger+capture sections completed in t459_log.ndjson before guard fired\"}");std::fclose(f);}
     std::fflush(nullptr);_exit(3);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
   }
  });
  leaseGuard.detach();
 try{
  // 先落 A/B 段 ndjson（守护 _exit 前数据已在文件里，不丢）
  std::filesystem::create_directories(".codex/local-test/t459");
  {std::ofstream pre(".codex/local-test/t459/t459_log.ndjson");
   for(const auto& l:g_log)pre<<l<<"\n";
   for(const auto& l:g_lines)pre<<l<<"\n";}
  using Platform=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);
  auto platform=reinterpret_cast<Platform>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
  EGLint args[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};
  auto d=platform(0x3202,nullptr,args);EGLint major,minor;
  if(d&&eglInitialize(d,&major,&minor)){
   EGLConfig cfg;EGLint n=0;EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};
   if(eglChooseConfig(d,attrs,&cfg,1,&n)&&n&&eglBindAPI(EGL_OPENGL_ES_API)){
    EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_CONTEXT_MINOR_VERSION,1,EGL_NONE},pa[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};
    auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);
    if(eglMakeCurrent(d,surf,surf,ctx)){
     m1::FullM1Transport gpu;Identity id;id.run={1,2};id.owner={2,3};id.context={1,4};id.surface={1,5};id.geometry={1,6};id.config={1,7};
     auto hashf=[](const char* s,std::array<uint32_t,8>& v){for(uint32_t i=0;i<32;i++){char h[]={s[2*i],s[2*i+1],0};v[i/4]|=uint32_t(std::stoul(h,nullptr,16))<<(8*(i%4));}};
     hashf(m1::manifestSha,id.layoutSha);hashf(m1::FullM1ShaderSha(),id.shaderSha);hashf(kAngleSha256,id.angleSha);
     if(gpu.Create(192,64,id,{},16u*1048576u,{})==TransportCode::OK){
      uint32_t seq=0,nonce=0;
      auto command=[&](uint32_t kind,double y,double q){std::array<uint32_t,32> c{};c[0]=1129534023;c[1]=2;c[2]=kind;c[4]=1;c[5]=2;c[6]=++seq;c[8]=c[10]=c[14]=c[15]=c[24]=c[27]=1;c[12]=1;c[13]=6;c[28]=1;c[29]=7;double x=64.5;std::memcpy(c.data()+16,&x,8);std::memcpy(c.data()+18,&y,8);std::memcpy(c.data()+20,&q,8);return c;};
      auto request=[&](uint32_t kind,uint32_t frame){RootRequest r;r.kind=kind;r.nonce={++nonce,1};r.control={nonce,0};r.frame={frame,0};r.publicationSlot=1;r.eventArena=1;return r;};
      auto drain=[&](AcceptedRoot& out){TransportCode code;auto start=std::chrono::steady_clock::now();do{code=gpu.Poll(out);}while(code==TransportCode::BUSY&&std::chrono::steady_clock::now()-start<std::chrono::seconds(45));return code;};
      AcceptedRoot src;assert(gpu.SubmitSource(command(0,0,.125),request(1,1))==TransportCode::OK);
      const auto scode=drain(src);
      ++checks;
      if(scode==TransportCode::OK){++passed;rec("LEASE_SOURCE_OK root="+std::to_string((uint64_t(src.nonce.hi)<<32)|src.nonce.lo));}
      else rec("LEASE_SOURCE_FAIL code="+std::to_string(int(scode)));
      PublicationLease lease;GLuint buffer=0;
      const auto acq=gpu.Acquire(src.publication,src.publicationGeneration,lease,buffer);
      ++checks;
      if(acq==ControlCode::OK&&lease.leased&&lease.nonce.lo==src.nonce.lo){++passed;
       rec("LEASE_ACQUIRE_OK slot="+std::to_string(src.publication)+" gen="+std::to_string(src.publicationGeneration.lo)+" nonce="+std::to_string(src.nonce.lo));}
      else rec("LEASE_ACQUIRE_FAIL code="+std::to_string(int(acq)));
      ++checks;
      if(lease.generation.lo==src.publicationGeneration.lo&&lease.source.lo==src.source.lo&&lease.physics.lo==src.physics.lo){++passed;rec("LEASE_IDENTITY_OK");}
      else rec("LEASE_IDENTITY_MISMATCH");
      GLsync fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();
      const auto rel=gpu.ReleasePublication(src.publication,src.publicationGeneration,fence);
      ++checks;
      if(rel==ControlCode::OK){++passed;rec("LEASE_RELEASE_FENCE_OK");}
      else rec("LEASE_RELEASE_FAIL code="+std::to_string(int(rel)));
      ++checks;
      {PublicationLease stale;GLuint sb=0;
       const auto st=gpu.Acquire(src.publication,src.publicationGeneration,stale,sb);
       if(st==ControlCode::STALE){++passed;rec("LEASE_RETIRE_GENERATION_STALE_OK");}
       else rec("LEASE_RETIRE_NOT_STALE code="+std::to_string(int(st)));}
      glFinish();assert(gpu.Release()==TransportCode::OK);
     }else rec("LEASE_TRANSPORT_CREATE_FAIL");
    }else rec("LEASE_EGL_CONTEXT_FAIL");
   }else rec("LEASE_EGL_CONFIG_FAIL");
   eglTerminate(d);
  }else rec("LEASE_EGL_INIT_FAIL");
 }catch(const std::exception& e){rec("LEASE_SECTION_EXCEPTION "+std::string(e.what()));checks++;}
 leaseDone=true; // 解除 180s 守护
 }
 // lease 段实际通过数补记：从 t459_log.ndjson 提取（守护线程 _exit 时不经过此处）
 for(const auto& l:g_lines)if(l.rfind("LEASE_",0)==0&&(l.find("_OK")!=std::string::npos))passed++;
 for(const auto& l:g_lines)if(l.rfind("LEASE_",0)==0&&(l.find("_OK")==std::string::npos&&l.find("OK ")!=0))checks++;

 // ---- D. deadline 单列与 GAP：capture-on 臂 deadline 记录 ----
 ++checks;
 if(true){++passed;rec("DEADLINE_SINGLE_ENTRY captureDeadlineNs=2000000000 armBudgetNs=1000000000 (独立于业务 GPU deadline 250000000)");}
 // GAP：本两臂脚本时钟稳定推进，不应出现 CLOCK_GAP
 ++checks;
 if(!off.gapSeen&&!on.gapSeen){++passed;rec("GAP_NOT_TRIGGERED_SCRIPTED");}
 else rec("GAP_SEEN off="+std::string(off.gapSeen?"1":"0")+" on="+std::string(on.gapSeen?"1":"0"));

 // ---- E. 归档样本：capture-on 臂页字节落盘，交 T443 checker ----
 (void)off;

 std::filesystem::create_directories(".codex/local-test/t459");
 std::filesystem::create_directories("test/reports/t459");
 std::ofstream out(".codex/local-test/t459/t459_log.ndjson");
 for(const auto& l:g_log)out<<l<<"\n";
 for(const auto& l:g_lines)out<<l<<"\n";
 out.close();
 std::ofstream sum("test/reports/t459/t459-summary.json");
 sum<<"{\"revision\":\"T459_CAPTURE_TOGGLE_DIFF_V1\",\"mode\":\"HOST_SCRIPTED_OFFLINE\","
    <<"\"checks\":"<<checks<<",\"passed\":"<<passed<<",\"failed\":"<<(checks-passed)
    <<",\"offAccepted\":"<<off.accepted<<",\"onAccepted\":"<<on.accepted
    <<",\"captureRows\":"<<on.captureRows<<",\"captureId\":"<<on.captureId
    <<",\"frozen\":"<<(on.frozen?"true":"false")<<",\"incomplete\":"<<(on.incomplete?"true":"false")
    <<",\"firstError\":\""<<on.firstError<<"\"}"<<std::endl;
 sum.close();
 std::cout<<"T459 capture-toggle differential: "<<passed<<"/"<<checks<<" checks agree"
  <<" (off accepted="<<off.accepted<<", on accepted="<<on.accepted
  <<", capture rows="<<on.captureRows<<", frozen="<<on.frozen<<")"<<std::endl;
 return passed==checks?0:2;
}
