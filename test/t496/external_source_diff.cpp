// T496 · 同源外线程 SOURCE 与异步采集对照（D1/D2 修订版，离线不使用真机）。
// 按 T494 §5 修订合同：选定 t492-m1-delivery 完整一致 M1 host 源树（route=m1，
// sourceCommit dcf105d…，T492 overlay），全部 -I 与 .cpp 指向该目录，禁止混编。
// 开跑前构建检查：showIncludes 清单 + 同选项 /Zs /d1reportSingleClassLayout
// 比对 driver TU 与实现 TU 的 FullM1Transport 布局，不一致拒开跑。
//
// D1 独立调度（§5.1）：独立调度 mutex/cv——owner 在预声明锁外测试 hook 发布
//   checkpoint 后等 permit；协调器等 checkpoint，放调度锁→拿 App 锁→设脚本
//   stamp/送动作组/落盘→放 App 锁→放行 owner。固定动作表含独立 pulseSeq、
//   SOURCE 实际字节、reserve/noop、END、有限尾部；每动作 append+flush 合法 JSON。
//   owner 只推进到预声明业务里程碑（Run.active/HasFlight/Queued 空闲快照）。
// D2 独立控制线程（§5.2）：真实 Flush/Wait busy 切点（GL 转发层注入暂停）在
//   App 锁外暂停，control 用生产 ReserveEvidenceLocked，保 request 完整终态。
// 断言唯一 ID；正控要求真实外线程 SOURCE 接受+非空 QUANTUM，空账 NOT_MEASURED。
// E 子集（lease 见证）不重跑，承接 T488 E12/12 旧观察（同源重建后只核必要见证）。
#include <iostream>
#include <cassert>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>
#include <iomanip>
#include <GLES3/gl31.h>
#include <EGL/egl.h>
#include <fstream>
// Host EGL stand-in: real ANGLE display, pbuffer "window" (same as t438/t475
// fixtures) so the owner loop's real GL path runs offline.
static EGLDisplay t496HostDisplay(EGLNativeDisplayType) {
    using Fn = EGLDisplay(EGLAPIENTRYP)(EGLenum, void*, const EGLint*);
    auto fn = reinterpret_cast<Fn>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    EGLint args[] = {0x3203, 0x3450, 0x3209, 0x3487, EGL_NONE};
    return fn(0x3202, nullptr, args);
}
static EGLSurface t496HostWindow(EGLDisplay d, EGLConfig cfg, EGLNativeWindowType, const EGLint*) {
    EGLint attrs[] = {EGL_WIDTH, 256, EGL_HEIGHT, 256, EGL_NONE};
    return eglCreatePbufferSurface(d, cfg, attrs);
}
extern "C" __eglMustCastToProperFunctionPointerType GL_APIENTRY t496GetProcAddress(const char*);
#define eglGetProcAddress t496GetProcAddress
#define eglGetDisplay t496HostDisplay
#define eglCreateWindowSurface t496HostWindow
#undef EGL_WINDOW_BIT
#define EGL_WINDOW_BIT EGL_PBUFFER_BIT
#define GRS_T456_BOOT
#define GRS_BRIDGE_HOST_TEST
#define GRS_BRIDGE_HOST_GL
#define GRS_PROGRESS_V2_EXPERIMENT
#define GRS_APP_M1
#include "../t492-m1-delivery/entry/src/main/cpp/gpu_resident/app_bridge.cpp"
#define glProgramBinary t496LegacyProgramBinary
#define glGetProgramBinary t496LegacyGetProgramBinary
#include "../t488/gl_wrappers.inc"
#include <GLES3/gl3.h>
#include "../t485/gl_exports.inc"
#undef glProgramBinary
#undef glGetProgramBinary
#include "t561_seams.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <map>
#include <algorithm>

// ---------- 结构化 JSONL：合法序列化、append+flush、唯一断言 ID ----------
static std::string g_runId, g_arm;
static std::mutex g_emitMutex;
static App* g_cutApp=nullptr;static std::mutex g_cutMu;static std::condition_variable g_cutCv;static bool g_cutEntered=false,g_cutReleased=false,g_cutStop=false,g_d2Mode=false;static std::atomic<bool> g_d2Frozen{false};static std::atomic<bool> g_cutArmed{false};
#ifdef GRS_T692_HOST_READBACK
#include "../t693/replay_sink.h"
static t693::Replay* g_replay = nullptr;
#endif
static std::string g_peSha;static uint64_t g_scriptBase=0;
static std::ofstream g_log;
static std::map<std::string, bool> g_asserted;
static std::map<std::string, int> g_seqCounter;
static std::ofstream g_stageLog;
static void StageEnterLog(const char* stage) {
    if (g_stageLog.is_open()) {
        std::ostringstream o;
        o << "{\"runId\":\"" << g_runId << "\",\"arm\":\"" << g_arm
          << "\",\"stage\":\"" << stage << "\",\"event\":\"ENTER\"}\n";
        g_stageLog << o.str() << std::flush;
    }
}
static void StageExitLog(const char* stage, const char* code) {
    if (g_stageLog.is_open()) {
        std::ostringstream o;
        o << "{\"runId\":\"" << g_runId << "\",\"arm\":\"" << g_arm
          << "\",\"stage\":\"" << stage << "\",\"event\":\"EXIT\",\"code\":\""
          << code << "\"}\n";
        g_stageLog << o.str() << std::flush;
    }
}

static std::string JsonEscape(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') o += "\\r";
        else if (c == '\t') o += "\\t";
        else if (static_cast<unsigned char>(c) < 0x20) {
            char b[8]; std::snprintf(b, sizeof(b), "\\u%04x", c); o += b;
        } else o += c;
    }
    return o;
}
static void Emit(const std::string& actionId, const std::string& phase,
                 uint64_t scriptNs, const std::string& detail) {
    std::lock_guard<std::mutex> emitLock(g_emitMutex);
    if (!g_log.is_open()) return;
    std::ostringstream o;
    o << "{\"runId\":\"" << g_runId << "\",\"arm\":\"" << g_arm
      << "\",\"actionId\":\"" << actionId << "\",\"phase\":\"" << phase
      << "\",\"clockDomain\":\"steady_real\",\"wallNs\":\"" << t496sink::WallNs()
      << "\",\"declaredScriptNs\":\""<<scriptNs<<"\",\"detail\":\"" << JsonEscape(detail) << "\"}\n";
    g_log << o.str() << std::flush;
}
static bool Assert(const std::string& id, bool pass, const std::string& detail) {
    std::lock_guard<std::mutex> emitLock(g_emitMutex);
    if (g_asserted.count(id)) return pass;
    g_asserted[id] = pass;
    if (g_log.is_open()) {
        std::ostringstream o;
        o << "{\"assertId\":\"" << id << "\",\"runId\":\"" << g_runId
          << "\",\"arm\":\"" << g_arm << "\",\"pass\":"
          << (pass ? "true" : "false") << ",\"detail\":\"" << JsonEscape(detail) << "\"}\n";
        g_log << o.str() << std::flush;
    }
    return pass;
}
static std::atomic<uint64_t> g_hostNowNs{1000000};

extern "C" void t496_shadow_hash_role(const char*,const char*);
extern "C" void t496_shadow_seed(const char*,const char*);
static std::map<std::string,std::string> CanonicalSources(){
 std::map<std::string,std::string> result{{"SOURCE",grs::programcache::Hash(grs::m1::FullSourceKernel())},{"INTERMEDIATE_QUANTUM",grs::programcache::Hash(grs::m1::QuantumKernel())},{"FINAL_QUANTUM",grs::programcache::Hash(grs::m1::QuantumCommitKernel())}};
 for(const auto& desc:grs::m1::ScenePrograms())result[std::string(desc.role)]=grs::programcache::Hash(desc.stages[0].bytes.data(),desc.stages[0].bytes.size())+grs::programcache::Hash(desc.stages[1].bytes.data(),desc.stages[1].bytes.size());
 return result;
}
static std::string ReceiptField(const std::string& line,const std::string& name){const auto prefix="\""+name+"\":\"";auto at=line.find(prefix);if(at==std::string::npos)return {};at+=prefix.size();auto end=line.find('"',at);return end==std::string::npos?std::string():line.substr(at,end-at);}
static void RegisterCachedSources(const std::string& filesDir){
 const auto known=CanonicalSources();const auto directory=std::filesystem::path(filesDir)/"gpu-program-cache/v1";
 for(const auto& entry:std::filesystem::directory_iterator(directory)){
  if(entry.path().extension()!=".bin")continue;const auto key=entry.path().stem().string();auto artifact=grs::programcache::Store().Get(key);if(!artifact)continue;
  auto receipt=entry.path();receipt.replace_extension(".jsonl");if(!std::filesystem::exists(receipt)||std::filesystem::file_size(receipt)>1048576)continue;
  const auto binary=grs::programcache::Hash(artifact->bytes.data(),artifact->bytes.size());std::ifstream in(receipt);std::string line;
  while(std::getline(in,line)){
   const auto role=ReceiptField(line,"role");const auto it=known.find(role);if(it==known.end()||ReceiptField(line,"key")!=key||(it->second.size()==64&&ReceiptField(line,"binarySHA")!=binary))continue;
   const auto& source=it->second;bool sourceMatch=source.size()==64?ReceiptField(line,"shaderSHA")==source:(line.find(source.substr(0,64))!=std::string::npos&&line.find(source.substr(64))!=std::string::npos);
   if(!sourceMatch)continue;t496_shadow_seed(binary.c_str(),source.c_str());Emit("BOOT","CACHE_SOURCE_BIND",t496sink::WallNs(),"role="+role+" key="+key+" binarySHA="+binary+" programSources="+source+" evidence=validated_artifact_receipt_not_GL_state");break;
  }
 }
}

// ---------- D1：独立调度 mutex/cv 固定动作表 ----------
enum class Ak { PULSE, SOURCE, RESERVE, END, TAIL_PULSE };
struct Act { uint32_t id; Ak kind; uint64_t scriptNs; uint32_t pulseSeq; uint32_t srcSeq; uint32_t phase; };

static std::vector<Act> BuildScript() {
    std::vector<Act> t;
    uint64_t ns = 1000000; uint32_t id = 0, pseq = 0, sseq = 0;
    auto pulse = [&](uint64_t at) { t.push_back({++id, Ak::PULSE, at, ++pseq, 0, 0}); };
    for (int i = 0; i < 6; ++i) pulse(ns += 16000000);
    uint64_t biz = ns + 16000000;
    t.push_back({++id, Ak::SOURCE, biz, 0, ++sseq, 0});
    pulse(biz);
    ns = biz;
    for (int i = 0; i < 10; ++i) pulse(ns += 16000000);
    ns += 16000000;
    t.push_back({++id, Ak::RESERVE, ns, 0, 0, 0});
    pulse(ns);
    for (int i = 0; i < 8; ++i) pulse(ns += 16000000);
    biz = ns + 16000000;
    t.push_back({++id, Ak::SOURCE, biz, 0, ++sseq, 1});
    pulse(biz);
    ns = biz;
    for (int i = 0; i < 12; ++i) pulse(ns += 16000000);
    ns += 16000000;
    t.push_back({++id, Ak::END, ns, 0, 0, 0});
    pulse(ns);
    for (int i = 0; i < 8; ++i) pulse(ns += 16000000);
    std::stable_sort(t.begin(), t.end(), [](const Act& a, const Act& b) {
        if (a.scriptNs != b.scriptNs) return a.scriptNs < b.scriptNs;
        auto rank = [](Ak k) { return k == Ak::SOURCE ? 0 : k == Ak::RESERVE ? 1
            : k == Ak::END ? 2 : 3; };
        return rank(a.kind) < rank(b.kind); });
    return t;
}

struct OwnerSnapshot {
    bool runActive=false, hasFlight=false, submitting=false;
    uint32_t queued=0; uint64_t accepted=0, sourceCommit=0, physicsCommit=0;
    uint64_t generated=0, revoked=0, overload=0, carry=0, clearedCarry=0;
    uint64_t pending=0, activeRemaining=0; uint32_t mailboxCount=0;
    uint64_t sourceEnqueuedNs=0; std::string phase, firstFailure, permission;
    bool evidenceArmed=false, evidenceFrozen=false; uint32_t captureRows=0;
    uint64_t beginPrefix=UINT64_MAX, endAccepted=0;
};

static OwnerSnapshot g_snap;
static std::mutex g_schedMu;
static std::condition_variable g_schedCv;
static bool g_ownerCheckpoint=false, g_permit=false, g_scriptDone=false;
#ifdef GRS_T703_HOST_WITNESS
static uint64_t g_checkpoint=0;
static t693w::Action g_delivered;
#endif
static uint64_t g_observedAccepted=0;

// owner hook：PumpBegin 发布 checkpoint 并等 permit（锁外）
static void D1OwnerHook(App& a, BridgeStage stage) {
    if (stage != BridgeStage::PumpBegin && stage != BridgeStage::Wake) return;
    {std::lock_guard<std::mutex> lock(a.mutex);if(!a.captureCommittedEpoch)return;}
    {
        std::lock_guard<std::mutex> lock(a.mutex);
        if(a.accepted!=g_observedAccepted){
          std::ostringstream ledger;EvidenceLedger::Of(a.progress).Json(ledger);
          Emit("ROOT","ACCEPTED_OBSERVED",g_hostNowNs.load(),"accepted="+std::to_string(a.accepted)+" source="+std::to_string(a.sourceCommit)+" physics="+std::to_string(a.physicsCommit)+" nonce="+std::to_string((uint64_t(a.rootNonce.hi)<<32)|a.rootNonce.lo)+" ledger="+ledger.str()+" token="+a.lastGoodControl);
          g_observedAccepted=a.accepted;
        }
        // 定长快照（owner 线程读 Run 合法）
        // T492 同源 App 的 Run/gpu 为 loop() 局部（非成员）；owner hook 无法跨作用域读。
        // 按 T494 §5.2 用现有成员冷接口：accepted/sourceCommit/evidence/mailbox。
        // Run 字段判据改为 optional（正控核心=sourceCommit>0）。
        g_snap.runActive = a.runQueue==0 && a.accepted>0;
        g_snap.hasFlight = a.runQueue>0;
        g_snap.submitting = false;
        g_snap.queued = a.runQueue;
        g_snap.accepted = a.accepted; g_snap.sourceCommit = a.sourceCommit;
        g_snap.physicsCommit = a.physicsCommit;
        g_snap.generated = a.progress.generated; g_snap.revoked = a.progress.revoked;
        g_snap.overload = a.progress.overload; g_snap.carry = a.progress.carry;
        g_snap.clearedCarry = a.progress.clearedCarry;
        g_snap.pending = a.progress.pending; g_snap.activeRemaining = a.progress.activeRemaining;
        g_snap.mailboxCount = uint32_t(a.count);
        g_snap.sourceEnqueuedNs = a.count ? a.sourceEnqueued[a.head] : 0;
        g_snap.phase = a.phase; g_snap.firstFailure = a.firstFailure;
        g_snap.permission = a.permission;
        g_snap.evidenceArmed = a.evidence && a.evidence->armed;
        g_snap.evidenceFrozen = a.evidence && a.evidence->frozen;
        g_snap.captureRows = a.evidence ? a.evidence->count : 0;
        g_snap.beginPrefix = a.evidence ? a.evidence->initial.accepted : UINT64_MAX;
        g_snap.endAccepted = a.evidence ? a.evidence->terminal.accepted : 0;
    }
    {   // 锁外交接
        std::unique_lock<std::mutex> lk(g_schedMu);
        g_ownerCheckpoint = true; g_permit = false;
#ifdef GRS_T703_HOST_WITNESS
        ++g_checkpoint;
#endif
        g_schedCv.notify_all();
        // T620 step4: non-blocking observer - checkpoint published, owner never
        // waits on the coordinator (old barrier only released at exit and could
        // wedge the owner). Coordinator acks asynchronously.
        g_schedCv.wait_for(lk,std::chrono::seconds(5),[&]{return g_permit||g_scriptDone;});
#ifdef GRS_T703_HOST_WITNESS
        if(g_permit&&g_delivered.first&&g_delivered.checkpoint==g_checkpoint){
            t693w::action=g_delivered;
            t693w::action.consumed=t496sink::WallNs();
        }
#endif
    }
}

// 协调器动作组执行（拿 App 锁→设 stamp→送→落盘→放锁）
static void DispatchActions(App& a, const std::vector<Act>& acts,
                            size_t& cursor, bool captureOn) {
    const uint64_t stamp = acts[cursor].scriptNs;
    while (cursor < acts.size() && acts[cursor].scriptNs == stamp) {
        const auto& act = acts[cursor];
        std::lock_guard<std::mutex> lock(a.mutex);
        const uint64_t received=t496sink::WallNs();g_hostNowNs.store(g_scriptBase+stamp); // Declared T494 fixed replay clock; wall/root deadlines remain separately observed.
#ifdef GRS_T703_HOST_WITNESS
        {
            std::lock_guard<std::mutex> checkpointLock(g_schedMu);
            t693w::action={uint64_t(act.id),uint64_t(act.id),g_scriptBase+stamp,received,g_checkpoint,0};
            if(g_delivered.checkpoint!=g_checkpoint)g_delivered=t693w::action;
            else g_delivered.last=act.id;
        }
#endif
        if (act.kind == Ak::PULSE || act.kind == Ak::TAIL_PULSE) {
            const char* code;{t561seams::Guard seam(t496sink::FN_SEAM_FIRST_PAGE_PULSE,std::string("session=")+std::to_string(a.progress.session)+" seq="+std::to_string(act.pulseSeq));code = a.PagePulseLocked(a.progress.session, act.pulseSeq+1, g_scriptBase+stamp);}
            Emit(std::to_string(act.id), "PULSE", stamp,
                 std::string("pulseSeq=") + std::to_string(act.pulseSeq) + " code=" + code);
        } else if (act.kind == Ak::SOURCE) {
            assert(a.SetSourceLocked(act.phase ? 1 : 0));
            a.ui.applied = a.ui.boundary; a.ui.sampleNs = g_scriptBase+stamp;
            a.ui.sampleValid = true; a.ui.sampleSeq++;
            std::array<uint32_t,32> w{};
            w[0]=1129534023; w[1]=2; w[2]=act.phase;
            w[4]=a.identity.run.lo; w[5]=a.identity.run.hi; w[6]=act.srcSeq;
            w[8]=a.input.lo; w[9]=a.input.hi; w[10]=uint32_t(a.ui.boundary);
            w[11]=uint32_t(a.ui.boundary>>32);
            w[12]=a.identity.geometry.lo; w[13]=a.identity.geometry.hi;
            w[14]=act.phase+1; w[15]=w[24]=act.srcSeq; w[27]=1;
            w[28]=a.identity.config.lo; w[29]=a.identity.config.hi;
            double x=64.5, y=act.phase?128.:0., m=.125;
            std::memcpy(w.data()+16,&x,8); std::memcpy(w.data()+18,&y,8);
            std::memcpy(w.data()+20,&m,8);
            // 实际 32U32 bytes 哈希落盘（SHA 简化：FNV-1a 32）
            uint32_t h=2166136261u;
            for (uint32_t v : w) for (int b=0;b<4;++b){h^=(v>>(8*b))&0xff;h*=16777619u;}
            const char* reply = a.OfferUiLocked(w);
            Emit(std::to_string(act.id), act.phase?"SOURCE_WATER":"SOURCE_SAND", stamp,
                 std::string("srcSeq=")+std::to_string(act.srcSeq)+" fnv="+
                 ([h]{std::ostringstream o;o<<std::hex<<h;return o.str();})()+
                 " reply="+reply+" mailbox="+std::to_string(a.count)+
                 " enqueuedNs="+std::to_string(a.count?a.sourceEnqueued[(a.head+a.count-1)%a.queue.size()]:0)+" payloadSHA="+grs::programcache::Hash(w.data(),w.size()*4)+" words="+([&]{std::ostringstream o;for(auto v:w)o<<v<<",";return o.str();})());
        } else if (act.kind == Ak::RESERVE) {
            if (!captureOn||g_d2Mode) { Emit(std::to_string(act.id),"RESERVE_OFF_NOOP",stamp,""); }
            else {
                EvidenceConfig c; c.packageSha256=g_peSha;
                c.expectedRevision="GRS_M1_PROGRESS_CLOCK_V2";
                c.epoch=a.epoch; c.runLo=a.identity.run.lo; c.runHi=a.identity.run.hi;
                c.pageSession=a.progress.session; c.durationNs=2000000000;
                const auto reply = a.ReserveEvidenceLocked(c);
                Emit(std::to_string(act.id),"RESERVE",stamp,reply.substr(0,160));
            }
        } else if (act.kind == Ak::END) {
            if (captureOn && a.evidence && a.evidence->armed) {
                const auto r = a.EndEvidenceLocked(a.evidence->id);
                Emit(std::to_string(act.id),"END",stamp,r.substr(0,160));
            } else Emit(std::to_string(act.id),"END_SKIP",stamp,"not armed/off");
        }
        ++cursor;
    }
#ifdef GRS_T703_HOST_WITNESS
    t693w::action={};
#endif
}

extern "C" void t496_before_original_flush(){
 if(!g_cutArmed.load()||!g_cutApp)return;
 {std::unique_lock<std::mutex> appLock(g_cutApp->mutex,std::try_to_lock);if(!appLock.owns_lock()||!g_cutApp->captureRootBusy)return;}
 if(!g_cutArmed.exchange(false))return;
 std::unique_lock<std::mutex> lock(g_cutMu);g_cutEntered=true;g_cutCv.notify_all();
 if(!g_cutCv.wait_for(lock,std::chrono::milliseconds(20),[]{return g_cutReleased;}))Emit("D2","CONTROL_HANDOFF_TIMEOUT",t496sink::WallNs(),"test wrapper BEFORE_ORIGINAL_GLFLUSH, not driver wait");
}
static void CaptureControl(App& a){
 {std::unique_lock<std::mutex> lock(g_cutMu);g_cutCv.wait_for(lock,std::chrono::seconds(120),[]{return g_cutEntered||g_cutStop;});if(!g_cutEntered){Emit("D2","NO_REAL_FLUSH_CUT",t496sink::WallNs(),"NOT_MEASURED");return;}}
 uint64_t request=0;
 {std::lock_guard<std::mutex> lock(a.mutex);EvidenceConfig c;c.packageSha256=g_peSha;c.expectedRevision="GRS_M1_PROGRESS_CLOCK_V2";c.epoch=a.epoch;c.runLo=a.identity.run.lo;c.runHi=a.identity.run.hi;c.pageSession=a.progress.session;c.durationNs=2000000000;
  Emit("D2","RESERVE_AT_FLUSH_BUSY",t496sink::WallNs(),"captureRootBusy="+std::to_string(a.captureRootBusy)+" Allowed="+std::to_string(a.AllowedLocked()));const auto reply=a.ReserveEvidenceLocked(c);request=a.captureRequest.id;Emit("D2","RESERVE_RAW",t496sink::WallNs(),reply);}
 {std::lock_guard<std::mutex> lock(g_cutMu);g_cutReleased=true;g_cutCv.notify_all();}
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
 while(std::chrono::steady_clock::now()<deadline){
  {std::lock_guard<std::mutex> lock(a.mutex);auto reply=a.ReadCaptureRequestLocked(request);Emit("D2","REQUEST_RAW",t496sink::WallNs(),reply);if(reply.find("FROZEN")!=std::string::npos)g_d2Frozen=true;if(reply.find("\"state\":\"RESERVED\"")==std::string::npos){if(a.evidence&&a.evidence->armed){const auto end=a.EndEvidenceLocked(a.evidence->id);Emit("D2","END_RAW",t496sink::WallNs(),end);if(a.evidence->frozen)g_d2Frozen=true;}return;}}
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
}
struct D1Result {
    OwnerSnapshot snap; bool delivered=false;
    size_t actions=0; std::string lastPhase;
};

static D1Result RunD1(bool captureOn, const std::vector<Act>& script) {
    D1Result R; // g_arm remains the same declared sink identity for this process.
    g_hostNowNs.store(t496sink::WallNs());
    g_scriptDone=g_permit=g_ownerCheckpoint=false;
    auto owner=std::make_unique<App>(false); auto& a=*owner;
    a.window=reinterpret_cast<void*>(1); a.nextProgressV2=true; a.tailProbeNext=true;
    a.hostNow=[](){ // T620 step2: real monotonic clock (script-clock profile declared separately)
        return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();};
    a.progress.BeginSession();
    {std::lock_guard<std::mutex> lock(a.mutex);a.RecoveryRequestLocked(true);}
    a.hostHook=&D1OwnerHook;
    g_cutApp=&a;g_cutEntered=g_cutReleased=g_cutStop=false;g_cutArmed=g_d2Mode;
    std::thread control;if(g_d2Mode)control=std::thread([&]{CaptureControl(a);});
    std::thread worker([&]{ a.loop(); });
    // 等 Create 完成：ANGLE JIT 编译三 compute+场景可达 8-15min（T475 fixture 同量级），
    // 上限 900s；期间 Emit 心跳每 30s 供外部守护读取。
    const auto resourceDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(240);
    for (int i=0;std::chrono::steady_clock::now()<resourceDeadline;++i){
        {std::lock_guard<std::mutex> lock(a.mutex);
         if (a.captureCommittedEpoch==a.epoch.load() && a.captureCommittedEpoch) break;}
        if (i%15000==0) { // T620 step2: coordinator iteration renamed + owner progress snapshot
           Emit("BOOT","COORDINATOR_WAIT_ITER",g_hostNowNs.load(),
              "coordinatorWaitIteration="+std::to_string(i)
              +"|ownerProgressSession="+std::to_string(a.progress.session)+"|epoch="+std::to_string(a.epoch.load())
              +"|captureCommittedEpoch="+std::to_string(a.captureCommittedEpoch));}
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    // T646 step6: this block only runs when captureCommittedEpoch is actually
    // set (loop broke on commit); on loop exhaustion emit TIMEOUT instead.
    if (a.captureCommittedEpoch != a.epoch.load() || !a.captureCommittedEpoch) {
        Emit("BOOT","RESOURCE_COMMIT_TIMEOUT",g_hostNowNs.load(),
             std::string("coordinatorWaitIteration exhausted without resource commit")
             +"|captureCommittedEpoch="+std::to_string(a.captureCommittedEpoch)
             +"|epoch="+std::to_string(a.epoch.load()));
        std::cerr << "RESOURCE_COMMIT_TIMEOUT" << std::endl;
        // bounded diagnostic snapshot before exit (step5: 2-3 samples)
        {   std::lock_guard<std::mutex> lk(a.mutex);
            std::ostringstream rs; a.RecoveryJson(rs,false);
            Emit("BOOT","DIAGNOSTIC_SNAPSHOT",g_hostNowNs.load(),
                 "recovery="+rs.str().substr(0,600)
                 +"|ownerProgressSession="+std::to_string(a.progress.session));
        }
        {std::lock_guard<std::mutex> l(a.mutex);a.quit=true;}g_scriptDone=true;g_schedCv.notify_all();{std::lock_guard<std::mutex> lock(g_cutMu);g_cutStop=true;g_cutCv.notify_all();}if(control.joinable())control.join();worker.join();return R;
    }
    // T620 step3: resource-commit reached -> capture recovery ledger + bind real
    // session + first pulse, via the production recovery entry points.
    {   std::lock_guard<std::mutex> lock(a.mutex);
        std::ostringstream rs; a.RecoveryJson(rs,false);
        Emit("BOOT","RESOURCE_COMMITTED",g_hostNowNs.load(),
             "recovery="+rs.str().substr(0,900)
             +"|ownerProgressSession="+std::to_string(a.progress.session));
        g_scriptBase=t496sink::WallNs();g_hostNowNs.store(g_scriptBase);a.hostNow=[](){return g_hostNowNs.load();};
        Emit("BOOT","CLOCK_PROFILE",g_scriptBase,"T494 scripted native/UI comparison, real transport root clock and parent wall observed separately; not device timegate qualification");
        const uint64_t serial=a.recovery.serial, e=a.recovery.epoch, page=1;
        const U64 run=a.recovery.candidate.run;
        if (serial) {
            uint64_t bound=a.RecoverySessionLocked(serial,e,run,page);
            Emit("BOOT","RECOVERY_BOUND",g_hostNowNs.load(),
                 "boundSession="+std::to_string(bound)
                 +"|serial="+std::to_string(serial)+"|epoch="+std::to_string(e));
            if (bound) {
                const char* reply=a.PagePulseLocked(bound,1,g_hostNowNs.load());
                Emit("BOOT","FIRST_PULSE_ACCEPTED",g_hostNowNs.load(),
                     "session="+std::to_string(bound)+"|seq=1|reply="+reply);
            } else {
                Emit("BOOT","RECOVERY_BIND_REJECTED",g_hostNowNs.load(),
                     "serial="+std::to_string(serial)+"|epoch="+std::to_string(e));
            }
        } else {
            Emit("BOOT","RECOVERY_LEDGER_EMPTY",g_hostNowNs.load(),"no serial at commit");
        }
        Emit("BOOT","READY_CHECK",g_hostNowNs.load(),
             "ownerProgressSession="+std::to_string(a.progress.session)
             +"|phase="+std::string(a.phase));
    }
    size_t cursor=0; R.actions=script.size();
    const auto startWall=std::chrono::steady_clock::now();
    uint64_t heldCheckpoints=0;
    const auto scriptDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(120);
    while (cursor < script.size()&&std::chrono::steady_clock::now()<scriptDeadline&&(!g_d2Mode||!g_d2Frozen.load())&&!t496sink::BudgetExceeded().load()) {
        bool timedOut=false;
        {   // 等 checkpoint（wall 守护 120s/动作组）
            std::unique_lock<std::mutex> lk(g_schedMu);
            if (!g_schedCv.wait_for(lk, std::chrono::seconds(5),
                    [&]{return g_ownerCheckpoint;})) timedOut=true;
            g_ownerCheckpoint=false;
        }
        if (timedOut) {
            Emit("WALL","GUARD_TIMEOUT",g_hostNowNs.load(),"no owner checkpoint in 5s");
            Assert("D1_WALL_GUARD",false,"owner stalled before script end");
            break;
        }
        // 业务里程碑门：仅在 idle/排空点送下一组（同 stamp 可多次 Pump/Poll）
        bool pending=false;
        {
            std::lock_guard<std::mutex> lock(a.mutex);
            pending=a.jobLive||a.runQueue||a.count||a.progress.activeRemaining||a.progress.pending;
            if(pending&&heldCheckpoints++==0)Emit("MILESTONE","HOLD_FIXED_STAMP",g_hostNowNs.load(),"jobLive="+std::to_string(a.jobLive)+" runQueue="+std::to_string(a.runQueue)+" mailbox="+std::to_string(a.count));
        }
        const auto kind=script[cursor].kind;
        const bool busyCut=t496sink::BinaryTrace()&&!g_d2Mode&&
            (kind==Ak::SOURCE||kind==Ak::RESERVE||kind==Ak::END);
        if(busyCut){
            Emit("MILESTONE","CONTROL_CHECKPOINT",g_hostNowNs.load(),
                "cursor="+std::to_string(cursor)+" ownerPending="+std::to_string(pending));
        }
        if(pending&&!busyCut){
            {std::lock_guard<std::mutex> lk(g_schedMu);g_permit=true;g_schedCv.notify_all();}
            std::this_thread::sleep_for(std::chrono::milliseconds(1));continue;
        }
        if(heldCheckpoints){Emit("MILESTONE","IDLE_AFTER_HOLD",g_hostNowNs.load(),"checkpoints="+std::to_string(heldCheckpoints));heldCheckpoints=0;}
        DispatchActions(a, script, cursor, captureOn);
        std::this_thread::sleep_for(std::chrono::milliseconds(g_d2Mode?12:1));
        {   // 放行 owner
            std::lock_guard<std::mutex> lk(g_schedMu);
            g_permit=true; g_schedCv.notify_all();
        }
    }
    R.delivered=cursor==script.size();
    if(!g_d2Mode)Assert("SCRIPT_COMPLETE",R.delivered,"delivered="+std::to_string(cursor)+" expected="+std::to_string(script.size()));
    // Finite drain: same fixed stamp, no extra pulse, real wall <=30s.
    {
        int drains=0;const auto drainDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
        while (std::chrono::steady_clock::now()<drainDeadline&&!t496sink::BudgetExceeded().load()) {
            std::unique_lock<std::mutex> lk(g_schedMu);
            if (!g_schedCv.wait_for(lk, std::chrono::seconds(1),
                    [&]{return g_ownerCheckpoint;})) break;
            g_ownerCheckpoint=false; ++drains;
            lk.unlock();
            {std::lock_guard<std::mutex> lock(a.mutex);
             if (!a.jobLive && a.runQueue==0 && a.count==0 && !a.progress.pending && !a.progress.activeRemaining) break;}
            lk.lock();g_permit=true; g_schedCv.notify_all();
            lk.unlock();std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    {   // quit 前快照（退出清理前）
        std::lock_guard<std::mutex> lock(a.mutex);
        R.snap=g_snap;R.snap.accepted=a.accepted;R.snap.sourceCommit=a.sourceCommit;R.snap.physicsCommit=a.physicsCommit;
        if(a.evidence&&a.evidence->frozen){for(uint32_t page=0;page<a.evidence->PageCount();++page)Emit("CAPTURE","PAGE_RAW",g_hostNowNs.load(),a.evidence->Page(page));}
        std::ostringstream progress;a.ProgressJsonLocked(progress);Emit("DRAIN","PROGRESS_RAW",g_hostNowNs.load(),progress.str());
        auto boot=a.bootProbe.Copy();boot456::Json(g_stageLog,boot,"T496_HOST_DIAGNOSTIC",uint64_t(getpid()));g_stageLog<<"\n"<<std::flush;
        Emit("DRAIN","PRE_QUIT_SNAPSHOT",g_hostNowNs.load(),
             "accepted="+std::to_string(a.accepted)+" sourceCommit="+std::to_string(a.sourceCommit)+
             " mailbox="+std::to_string(a.count)+" runQueue="+std::to_string(a.runQueue)+" phase="+a.phase+" firstFailure="+a.firstFailure+" permission="+a.permission);
        a.quit=true;
    }
    {std::lock_guard<std::mutex> lock(g_schedMu);g_scriptDone=true;g_permit=true;g_schedCv.notify_all();}
    {std::lock_guard<std::mutex> lock(g_cutMu);g_cutStop=true;g_cutReleased=true;g_cutCv.notify_all();}
    if(control.joinable())control.join();worker.join();g_cutApp=nullptr;
    R.lastPhase=R.snap.phase;
    return R;
}

// 2026-11-14 运行时阻断记录：Create 在 ANGLE 上 >52min 未完成（iter 210000/450000，
// CPU 活跃、心跳正常）。T475 同构 fixture 8min 完成；T492 overlay（recovery+scene-cache）
// 疑似与 HOST_TEST 交互引入卡点。下一轮：用暖缓存种子
// （test/t492/build_app.cjs 第4参数，同 T485-app-m1-exact cache）跳过 compute JIT，
// 仅 graphics 冷编译，验证是否 Create 路径本身卡死。
// CREATE_ONLY 三臂（T518 §3）：argv[1] = C0 | C1 | C2 | D1（旧 D1 两臂模式保留）。
//  C0: 空 cache（纯冷）；C1: 仅 3 compute bin；C2: 全部 6 role bin。
//  每臂独立新进程+新输出目录；Store().Configure 在 owner 启动前完成；
//  900s 外部 watchdog 由 run_create_only.cmd 负责（不在进程内延长）。
//  GL 编译链 ENTER/EXIT 落盘见 gl_sink.inc；首个 ENTER 无 EXIT 即首个未返回调用。
static int RunCreateOnly(const std::string& arm) {
    g_arm = "create_" + arm;
    const std::string seedSrc = ".codex/local-test/t485-app-m1-exact/cache/gpu-program-cache/v1";
    const std::string filesDir = ".codex/local-test/t496/" + t496sink::AttemptId() + "-files";
    std::filesystem::remove_all(filesDir);
    std::filesystem::create_directories(filesDir + "/gpu-program-cache/v1");
    std::string packageSource = "EMPTY_CACHE";
    if (arm == "C1" || arm == "C2") {
        // compute 三 key：SOURCE=1028e02a INTERMEDIATE=9062993a FINAL=545a3f5f
        const char* computeKeys[] = {"1028e02a", "9062993a", "545a3f5f"};
        const char* gfxKeys[] = {"834a1766", "cd57d4ab", "e3c4f777"};
        auto copyByPrefix = [&](const char* prefix) {
            for (auto& e : std::filesystem::directory_iterator(seedSrc)) {
                const auto name = e.path().filename().string();
                if (name.rfind(prefix, 0) == 0)
                    std::filesystem::copy_file(e.path(), filesDir + "/gpu-program-cache/v1/" + name,
                                               std::filesystem::copy_options::overwrite_existing);
            }
        };
        for (auto k : computeKeys) copyByPrefix(k);
        packageSource = "SEEDED_COMPUTE3";
        if (arm == "C2") { for (auto k : gfxKeys) copyByPrefix(k); packageSource = "SEEDED_ALL6"; }
    }
    Emit("BOOT", "CREATE_ONLY_START", 0,
         "arm=" + arm + " filesDir=" + filesDir + " packageSource=" + packageSource);

    // Store.Configure 在 owner 启动前完成并验证（T518 §3.2）
    const bool configured = programcache::Store().Configure(filesDir);
    Emit("BOOT", "STORE_CONFIGURED", 0,
         std::string("ok=") + (configured ? "true" : "false") +
         " entries=" + std::to_string(programcache::Store().Count()));
    if (!configured) { Assert("CREATE_STORE_CONFIGURED", false, "Configure failed"); return 2; }
    Assert("CREATE_STORE_CONFIGURED", true,
           "entries=" + std::to_string(programcache::Store().Count()));
    RegisterCachedSources(filesDir);
    programcache::packageSource = "T496_" + arm + ":" + packageSource;

    // owner 启动（同 D1 路径），等 captureCommittedEpoch（资源 commit）或 900s
    g_hostNowNs.store(t496sink::WallNs());
    auto owner = std::make_unique<App>(false); auto& a = *owner;
    a.window = reinterpret_cast<void*>(1); a.nextProgressV2 = true; a.tailProbeNext = true;
    a.hostNow = []() { return t496sink::WallNs(); };
    a.progress.BeginSession();
    {std::lock_guard<std::mutex> lock(a.mutex);a.RecoveryRequestLocked(true);Emit("BOOT","RECOVERY_REQUESTED",t496sink::WallNs(),"serial="+std::to_string(a.recovery.serial));}
    a.hostHook = nullptr; // CREATE_ONLY has no coordinator checkpoint handshake; real guard below.
    const auto startWall = std::chrono::steady_clock::now();
    std::thread worker([&] { a.loop(); });
    bool committed = false;
    uint64_t lastCaptureEpoch = 0; bool lastResourcesReady = false;
    for (int i = 0; std::chrono::steady_clock::now()-startWall<std::chrono::seconds(240); ++i) {
        { std::lock_guard<std::mutex> lock(a.mutex);
          // T561 seams: observed transitions, recorded with actual values.
          if (a.v2ResourcesReady != lastResourcesReady) {
            lastResourcesReady = a.v2ResourcesReady;
            t561seams::Observe(t496sink::FN_SEAM_RESOURCE_COMMIT,
              std::string("v2ResourcesReady=") + (a.v2ResourcesReady ? "true" : "false"));
          }
          if (a.captureCommittedEpoch != lastCaptureEpoch) {
            lastCaptureEpoch = a.captureCommittedEpoch;
            t561seams::Observe(t496sink::FN_SEAM_CAPTURE_EPOCH,
              std::string("epoch=") + std::to_string(a.captureCommittedEpoch));
          }
          if (a.captureCommittedEpoch == a.epoch && a.captureCommittedEpoch && a.v2ResourcesReady) { committed = true;
            t561seams::Observe(t496sink::FN_SEAM_READY, std::string("phase=") + a.phase);
            break; } }
        if(i%250==0){auto probe=a.bootProbe.Copy();std::ostringstream observed;boot456::Json(observed,probe,"T496_HOST_DIAGNOSTIC",uint64_t(getpid()));g_stageLog<<observed.str()<<"\n"<<std::flush;}
        if (i % 15000 == 0) Emit("BOOT", "CREATE_WAIT", g_hostNowNs.load(),
             "iter=" + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    if(committed){std::lock_guard<std::mutex> lock(a.mutex);
      Emit("BOOT","RESOURCE_COMMITTED",t496sink::WallNs(),"real capture epoch and v2ResourcesReady");
      const auto bound=a.RecoverySessionLocked(a.recovery.serial,a.recovery.epoch,a.recovery.candidate.run,1);
      Emit("BOOT","RECOVERY_BOUND",t496sink::WallNs(),"session="+std::to_string(bound));
      const char* result=bound?a.PagePulseLocked(bound,1,t496sink::WallNs()):"SESSION_NOT_BOUND";
      Emit("BOOT","FIRST_PULSE_RETURN",t496sink::WallNs(),result);
      a.TryReadyV2Locked();committed=bound&&std::string(result)=="PAGE_PULSE_ACCEPTED"&&a.AllowedLocked();
      Emit("BOOT","READY_GUARD",t496sink::WallNs(),committed?"AllowedLocked=true":"AllowedLocked=false");
    }
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startWall).count();
    Emit("BOOT", committed ? "CREATE_COMMITTED" : "CREATE_TIMEOUT", g_hostNowNs.load(),
         "wallMs=" + std::to_string(elapsedMs));
    t561seams::Observe(t496sink::FN_SEAM_TRANSPORT_CREATE,
        std::string("committed=") + (committed ? "true" : "false") + " wallMs=" + std::to_string(elapsedMs));
    t561seams::Observe(t496sink::FN_SEAM_SCENE_PREPARE,
        std::string("phase=") + a.phase + " firstFailure=" + a.firstFailure);
    Assert("CREATE_RESOURCE_COMMIT_" + arm, committed,
           "wallMs=" + std::to_string(elapsedMs) +
           " epoch=" + std::to_string(a.captureCommittedEpoch));
    // 有限退出：设 quit+scriptDone+permit 并 notify（修 drain 自锁/卡 owner）
    {
        std::lock_guard<std::mutex> lk(g_schedMu);
        g_scriptDone = true; g_permit = true;
        g_schedCv.notify_all();
    }
    { std::lock_guard<std::mutex> lock(a.mutex); a.quit = true; }
    worker.join();
    return committed ? 0 : 1;
}

extern "C" bool t496_sink_initialize(const char*, const char*, const char*); // gl_forward_sink TU
static int RunCreateOnlyDispatch(const std::string& arm, const std::string& attemptArg) {
    const std::string attemptId = attemptArg.empty()
        ? arm + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
        : attemptArg;
    g_runId = "t496-" + attemptId;g_arm="create_"+arm;
    { // T547 §3: sink identity injection + attempt-unique trace file (create-new)
        if (!t496_sink_initialize(g_runId.c_str(), ("create_" + arm).c_str(), attemptId.c_str())) {
            std::cerr << "SINK_INITIALIZE_FAILED (attempt file reuse?)" << std::endl;
            return 5;
        }
    }
    std::filesystem::create_directories(".codex/local-test/t496");
    std::filesystem::create_directories("test/reports/t496");
    const std::string tag = "t496_log_" + attemptId + ".ndjson";
    g_log.open(".codex/local-test/t496/" + tag, std::ios::app);
    g_stageLog.open(".codex/local-test/t496/t496-stages_" + attemptId + ".ndjson", std::ios::app);
    int rc=0;
    if(arm=="D1_OFF"||arm=="D1_ON"||arm=="D2"){
      const auto files=std::string(".codex/local-test/t496/")+attemptId+"-files";std::filesystem::create_directories(files+"/gpu-program-cache/v1");
      const auto seed=std::filesystem::path(".codex/local-test/t496/t657r3-files/gpu-program-cache/v1");
      for(auto& e:std::filesystem::directory_iterator(seed))if(e.is_regular_file())std::filesystem::copy_file(e.path(),std::filesystem::path(files)/"gpu-program-cache/v1"/e.path().filename());
      if(!programcache::Store().Configure(files))return 7;RegisterCachedSources(files);
      g_d2Mode=arm=="D2";auto script=BuildScript();if(g_d2Mode)script.erase(std::remove_if(script.begin(),script.end(),[](const Act& a){return a.kind==Ak::SOURCE;}),script.end());auto result=RunD1(arm!="D1_OFF",script);
      const bool source=result.snap.sourceCommit>0,quantum=result.snap.physicsCommit>0;
      if(!g_d2Mode)Assert("EXTERNAL_SOURCE_NONEMPTY",source,"source="+std::to_string(result.snap.sourceCommit));Assert("QUANTUM_NONEMPTY",quantum,"physics="+std::to_string(result.snap.physicsCommit));rc=g_d2Mode?(g_d2Frozen?0:9):(source&&quantum&&result.delivered?0:8);
    }else rc=RunCreateOnly(arm);
    std::cout << "T496 CREATE_ONLY " << arm << ": rc=" << rc << std::endl;
#ifdef GRS_T692_HOST_READBACK
    if(g_replay&&!g_replay->store(".codex/local-test/t496/t693-replay-"+attemptId))rc=11;
#endif
    t496sink::CloseBinaryTrace();
    return t496sink::BudgetExceeded().load()?10:rc;
}

extern "C" void t496_shadow_role(const char*,const char*);
int main(int argc, char** argv) {
    {std::ifstream exe(argv[0],std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(exe)),{});g_peSha=grs::programcache::Hash(bytes);}
    t496_shadow_role(grs::m1::FullSourceKernel(),"SOURCE");
    t496_shadow_role(grs::m1::QuantumKernel(),"INTERMEDIATE_QUANTUM");
    t496_shadow_role(grs::m1::QuantumCommitKernel(),"FINAL_QUANTUM");
    for(const auto& item:CanonicalSources())if(item.second.size()==128)t496_shadow_hash_role(item.second.c_str(),item.first.c_str());
#ifdef GRS_T692_HOST_READBACK
    std::unique_ptr<t693::Replay> replay;
    if(std::getenv("T693_REPLAY")){
        replay=std::make_unique<t693::Replay>();
        if(!replay->install())return 11;
        g_replay=replay.get();
    }
#endif
    if (argc == 3) {
        const std::string arm = argv[1];
        if (arm == "C0" || arm == "C1" || arm == "C2" || arm=="D1_OFF" || arm=="D1_ON" || arm=="D2") return RunCreateOnlyDispatch(arm, argc > 2 ? argv[2] : "");
        // T646 step1: predeclared modes only — unknown params must exit non-zero,
        // never silently fall through to default D1 (v6 ran D1_OFF by accident).
        std::fprintf(stderr, "T646_UNKNOWN_MODE:%s\n", arm.c_str());
        return 2;
    }
    if(argc!=3){std::cerr<<"EXPLICIT_MODE_AND_ATTEMPT_REQUIRED";return 2;}
    return 2;
}
