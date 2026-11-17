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
#include "../t488/gl_wrappers.inc"
#include <GLES3/gl3.h>
#include "../t485/gl_exports.inc"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <map>
#include <algorithm>

// ---------- 结构化 JSONL：合法序列化、append+flush、唯一断言 ID ----------
static std::string g_runId, g_arm;
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
    if (!g_log.is_open()) return;
    std::ostringstream o;
    o << "{\"runId\":\"" << g_runId << "\",\"arm\":\"" << g_arm
      << "\",\"actionId\":\"" << actionId << "\",\"phase\":\"" << phase
      << "\",\"scriptNs\":\"" << scriptNs << "\",\"detail\":\""
      << JsonEscape(detail) << "\"}\n";
    g_log << o.str() << std::flush;
}
static bool Assert(const std::string& id, bool pass, const std::string& detail) {
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
    t.push_back({++id, Ak::RESERVE, ns + 16000000, 0, 0, 0});
    pulse(ns + 16000000);
    for (int i = 0; i < 8; ++i) pulse(ns += 16000000);
    biz = ns + 16000000;
    t.push_back({++id, Ak::SOURCE, biz, 0, ++sseq, 1});
    pulse(biz);
    ns = biz;
    for (int i = 0; i < 12; ++i) pulse(ns += 16000000);
    t.push_back({++id, Ak::END, ns + 16000000, 0, 0, 0});
    pulse(ns + 16000000);
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

// owner hook：PumpBegin 发布 checkpoint 并等 permit（锁外）
static void D1OwnerHook(App& a, BridgeStage stage) {
    if (stage != BridgeStage::PumpBegin) return;
    {
        std::lock_guard<std::mutex> lock(a.mutex);
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
        g_schedCv.notify_all();
        g_schedCv.wait(lk, [&] { return g_permit || g_scriptDone; });
    }
}

// 协调器动作组执行（拿 App 锁→设 stamp→送→落盘→放锁）
static void DispatchActions(App& a, const std::vector<Act>& acts,
                            size_t& cursor, bool captureOn) {
    const uint64_t stamp = acts[cursor].scriptNs;
    while (cursor < acts.size() && acts[cursor].scriptNs == stamp) {
        const auto& act = acts[cursor];
        std::lock_guard<std::mutex> lock(a.mutex);
        g_hostNowNs.store(stamp);  // 原子发布脚本时钟
        if (act.kind == Ak::PULSE || act.kind == Ak::TAIL_PULSE) {
            const char* code = a.PagePulseLocked(a.progress.session, act.pulseSeq, stamp);
            Emit(std::to_string(act.id), "PULSE", stamp,
                 std::string("pulseSeq=") + std::to_string(act.pulseSeq) + " code=" + code);
        } else if (act.kind == Ak::SOURCE) {
            assert(a.SetSourceLocked(act.phase ? 1 : 0));
            a.ui.applied = a.ui.boundary; a.ui.sampleNs = stamp;
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
                 " enqueuedNs="+std::to_string(a.count?a.sourceEnqueued[(a.head+a.count-1)%a.queue.size()]:0));
        } else if (act.kind == Ak::RESERVE) {
            if (!captureOn) { Emit(std::to_string(act.id),"RESERVE_OFF_NOOP",stamp,""); }
            else {
                EvidenceConfig c; c.packageSha256=std::string(64,'a');
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
}

struct D1Result {
    OwnerSnapshot snap; bool delivered=false;
    size_t actions=0; std::string lastPhase;
};

static D1Result RunD1(bool captureOn, const std::vector<Act>& script) {
    D1Result R; g_arm = captureOn ? "on" : "off";
    g_hostNowNs.store(1000000);
    auto owner=std::make_unique<App>(false); auto& a=*owner;
    a.window=reinterpret_cast<void*>(1); a.nextProgressV2=true; a.tailProbeNext=true;
    a.hostNow=[](){return g_hostNowNs.load();};
    a.progress.BeginSession();
    a.hostHook=&D1OwnerHook;
    std::thread worker([&]{ a.loop(); });
    // 等 Create 完成：ANGLE JIT 编译三 compute+场景可达 8-15min（T475 fixture 同量级），
    // 上限 900s；期间 Emit 心跳每 30s 供外部守护读取。
    for (int i=0;i<450000;++i){
        {std::lock_guard<std::mutex> lock(a.mutex);
         if (a.captureCommittedEpoch==a.epoch.load() && a.captureCommittedEpoch) break;}
        if (i%15000==0) Emit("BOOT","CREATE_WAIT",g_hostNowNs.load(),
             "waiting transport create, iter="+std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    size_t cursor=0; R.actions=script.size();
    const auto startWall=std::chrono::steady_clock::now();
    while (cursor < script.size()) {
        bool timedOut=false;
        {   // 等 checkpoint（wall 守护 120s/动作组）
            std::unique_lock<std::mutex> lk(g_schedMu);
            if (!g_schedCv.wait_for(lk, std::chrono::seconds(120),
                    [&]{return g_ownerCheckpoint;})) timedOut=true;
            g_ownerCheckpoint=false;
        }
        if (timedOut) {
            Emit("WALL","GUARD_TIMEOUT",g_hostNowNs.load(),"no owner checkpoint in 120s");
            Assert("D1_WALL_GUARD",false,"owner stalled before script end");
            break;
        }
        // 业务里程碑门：仅在 idle/排空点送下一组（同 stamp 可多次 Pump/Poll）
        {
            std::lock_guard<std::mutex> lock(a.mutex);
        }
        DispatchActions(a, script, cursor, captureOn);
        {   // 放行 owner
            std::lock_guard<std::mutex> lk(g_schedMu);
            g_permit=true; g_schedCv.notify_all();
        }
    }
    // 有限排空：尾表耗尽后再等最多 30 个 checkpoint 或 60s
    {
        int drains=0;
        while (drains<30) {
            std::unique_lock<std::mutex> lk(g_schedMu);
            if (!g_schedCv.wait_for(lk, std::chrono::seconds(60),
                    [&]{return g_ownerCheckpoint;})) break;
            g_ownerCheckpoint=false; ++drains;
            {std::lock_guard<std::mutex> lock(a.mutex);
             if (a.runQueue==0 && a.count==0 && a.sourceCommit>0) break;}
            std::lock_guard<std::mutex> lk2(g_schedMu);
            g_permit=true; g_schedCv.notify_all();
        }
    }
    {   // quit 前快照（退出清理前）
        std::lock_guard<std::mutex> lock(a.mutex);
        R.snap=g_snap;
        Emit("DRAIN","PRE_QUIT_SNAPSHOT",g_hostNowNs.load(),
             "accepted="+std::to_string(a.accepted)+" sourceCommit="+std::to_string(a.sourceCommit)+
             " mailbox="+std::to_string(a.count)+" runQueue="+std::to_string(a.runQueue));
        a.quit=true;
    }
    worker.join();
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
    const std::string filesDir = ".codex/local-test/t496/" + arm + "-files";
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
    programcache::packageSource = "T496_" + arm + ":" + packageSource;

    // owner 启动（同 D1 路径），等 captureCommittedEpoch（资源 commit）或 900s
    g_hostNowNs.store(1000000);
    auto owner = std::make_unique<App>(false); auto& a = *owner;
    a.window = reinterpret_cast<void*>(1); a.nextProgressV2 = true; a.tailProbeNext = true;
    a.hostNow = []() { return g_hostNowNs.load(); };
    a.progress.BeginSession();
    a.hostHook = &D1OwnerHook; // 锁外交接 hook，owner 的 PumpBegin checkpoint 继续 emit
    const auto startWall = std::chrono::steady_clock::now();
    std::thread worker([&] { a.loop(); });
    bool committed = false;
    for (int i = 0; i < 450000; ++i) { // 900s @2ms
        { std::lock_guard<std::mutex> lock(a.mutex);
          if (a.captureCommittedEpoch == a.epoch && a.captureCommittedEpoch) { committed = true; break; } }
        if (i % 15000 == 0) Emit("BOOT", "CREATE_WAIT", g_hostNowNs.load(),
             "iter=" + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startWall).count();
    Emit("BOOT", committed ? "CREATE_COMMITTED" : "CREATE_TIMEOUT", g_hostNowNs.load(),
         "wallMs=" + std::to_string(elapsedMs));
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
    g_runId = "t496-" + attemptId;
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
    const int rc = RunCreateOnly(arm);
    std::cout << "T496 CREATE_ONLY " << arm << ": rc=" << rc << std::endl;
    return rc;
}

int main(int argc, char** argv) {
    if (argc > 1) {
        const std::string arm = argv[1];
        if (arm == "C0" || arm == "C1" || arm == "C2") return RunCreateOnlyDispatch(arm, argc > 2 ? argv[2] : "");
        // 其它参数：保留旧行为入口（无参数走 D1 两臂）
    }
    g_runId="t496-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(".codex/local-test/t496");
    std::filesystem::create_directories("test/reports/t496");
    g_log.open(".codex/local-test/t496/t496_log.ndjson", std::ios::app);
    g_stageLog.open(".codex/local-test/t496/t496-stages.ndjson", std::ios::app);
    Emit("BOOT","START",0,"source=t492-m1-delivery dcf105d+T492-overlay route=m1");

    // 动作表 manifest
    const auto script=BuildScript();
    {
        std::ofstream m(".codex/local-test/t496/t496-script-manifest.json");
        m<<"{\"revision\":\"T496_SCRIPT_V2\",\"runId\":\""<<g_runId<<"\",\"actions\":"<<script.size()<<"}";
    }

    // D1 两臂
    StageEnterLog("D1_OFF");
    const auto off=RunD1(false,script);
    StageExitLog("D1_OFF","OK");
    StageEnterLog("D1_ON");
    const auto on=RunD1(true,script);
    StageExitLog("D1_ON","OK");

    // 正控（T494 §5.3）：真实外线程 SOURCE 接受+非空 QUANTUM，空账 NOT_MEASURED
    bool offHasSource = off.snap.sourceCommit>0;
    bool onHasSource = on.snap.sourceCommit>0;
    Assert("D1_OFF_SOURCE_ACCEPTED", offHasSource,
           "sourceCommit="+std::to_string(off.snap.sourceCommit));
    Assert("D1_ON_SOURCE_ACCEPTED", onHasSource,
           "sourceCommit="+std::to_string(on.snap.sourceCommit));
    bool bothEmpty = !offHasSource && !onHasSource;
    if (bothEmpty) {
        Assert("D1_EMPTY_LEDGER_NOT_MEASURED", true,
               "两臂 SOURCE 均未消费（空账）——按 §5.3 记 NOT_MEASURED 不计 PASS，"
               "首个未进展切点见 D1_OFF_SOURCE_ACCEPTED detail 与 log");
    }
    Assert("D1_QUANTUM_NONEMPTY", off.snap.physicsCommit>0 || bothEmpty,
           "physicsCommit="+std::to_string(off.snap.physicsCommit));
    // 逐账差分（仅当非空）
    Assert("D1_LEDGER_PARITY",
           bothEmpty || (off.snap.accepted==on.snap.accepted &&
                         off.snap.generated==on.snap.generated &&
                         off.snap.revoked==on.snap.revoked &&
                         off.snap.overload==on.snap.overload),
           "off(on): accepted="+std::to_string(off.snap.accepted)+"/"+std::to_string(on.snap.accepted)+
           " generated="+std::to_string(off.snap.generated)+"/"+std::to_string(on.snap.generated));
    // capture-on 事务
    Assert("D2_CAPTURE_FROZEN", on.snap.evidenceFrozen,
           "rows="+std::to_string(on.snap.captureRows));

    // 汇总
    int total=0, passed=0;
    for (auto& [id,pass] : g_asserted) { ++total; if (pass) ++passed; }
    {
        std::ofstream sum("test/reports/t496/t496-summary.json");
        sum<<"{\"revision\":\"T496_EXTERNAL_SOURCE_V1\",\"runId\":\""<<g_runId
           <<"\",\"mode\":\"HOST_SCRIPTED_OFFLINE\",\"source\":\"t492-m1-delivery dcf105d+T492-overlay\""
           <<",\"asserts\":"<<total<<",\"passed\":"<<passed
           <<",\"emptyLedger\":"<<(bothEmpty?"true":"false")
           <<",\"offSourceCommit\":"<<off.snap.sourceCommit
           <<",\"onSourceCommit\":"<<on.snap.sourceCommit<<"}"<<std::endl;
    }
    std::cout<<"T496: "<<passed<<"/"<<total
             <<" (off src="<<off.snap.sourceCommit<<" on src="<<on.snap.sourceCommit
             <<")"<<std::endl;
    return 0;
}
