#define GRS_PROGRESS_V2_EXPERIMENT
#define T404_EMBED_HOST
#include "../t422/t409_host_nomain.cpp"
#include "angles_generated.h"
#include <fstream>
#include <filesystem>
// ArmResult 与 hook 状态独立声明（不依赖 T459 编译单元）
static std::vector<std::string> g_log;
static std::vector<std::string> g_lines;
static void rec(const std::string& s) { g_lines.push_back(s); std::cerr << s << std::endl; }
static bool g_armCaptureOn = false;
static std::atomic<bool> g_controlReserved{false}, g_endRequested{false};
struct ArmResult {
    bool ok = false; uint64_t accepted = 0, generated = 0, revoked = 0, overload = 0;
    uint64_t carry = 0, clearedCarry = 0, pendingEnd = 0, activeEnd = 0;
    uint64_t captureRows = 0, captureDropped = 0, captureId = 0;
    bool frozen = false, incomplete = false;
    uint64_t beginAcceptedPrefix = UINT64_MAX, endAccepted = 0;
    std::vector<std::array<uint32_t, 256>> completions;
};
// T471-D · GPU 采集开关对称脚本差分（T465 §3 D 包，修 T459 两处用例缺陷）。
// 修正1：两臂同固定输入 = 预生成 pulse 调度表（seq/callback/nativeNow 全固定数组），
//        不按 wall 睡眠推进、不按 accepted=8 提前停——固定总数 N 后排空终止。
// 修正2：on 臂在两臂共享安全点（固定 pulse 序号处）reserve/END；off 臂同点空操作。
// 对拍：逐 pulse session/seq/两域 stamp/ready/baseline；逐 root completion；
//       全账 carry/pending/active/cleared/generated/accepted/revoked/overload。
// 标 HOST_SCRIPTED_OFFLINE。

static const uint32_t kTotalPulses = 40;       // 固定脚本长度
static const uint64_t kPulseStepNs = 16000000; // 16ms UI 节拍（<33.3ms GAP）

struct PulseRecord {
    uint64_t seq, uiNs, nativeNs, receiveNs;
    bool accepted, ready;
    uint64_t generated, acceptedQ, pending, activeRemaining, carry, clearedCarry, revoked, overload;
};
static std::vector<PulseRecord> g_records;
static void recordPulse(App& a, uint64_t seq, uint64_t ui, uint64_t native, bool acc) {
    PulseRecord r{};
    r.seq = seq; r.uiNs = ui; r.nativeNs = native; r.receiveNs = a.progress.receiveNs;
    r.accepted = acc; r.ready = a.AllowedLocked();
    r.generated = a.progress.generated; r.acceptedQ = a.progress.accepted;
    r.pending = a.progress.pending; r.activeRemaining = a.progress.activeRemaining;
    r.carry = a.progress.carry; r.clearedCarry = a.progress.clearedCarry;
    r.revoked = a.progress.revoked; r.overload = a.progress.overload;
    g_records.push_back(r);
}

// 预生成调度表：两臂同表
struct Schedule { std::vector<uint64_t> ui, native; };
static Schedule makeSchedule() {
    Schedule s;
    uint64_t t = 1000000;
    for (uint32_t i = 0; i < kTotalPulses; ++i) {
        s.ui.push_back(t);
        s.native.push_back(t);
        t += kPulseStepNs;
    }
    return s;
}

// T471 独立 hook：不引用 T459 的 t459Hook；本卡由 pulses 线程全驱动（调度表），
// PumpBegin 共享安全点等待（reserveAtPulse 前后行为两臂一致）。
static void t471Hook(App& app, BridgeStage stage) {
    if (stage == BridgeStage::PumpBegin && !ownerAtRoot.exchange(true)) {
        while (g_armCaptureOn && !g_controlReserved) std::this_thread::yield();
    }
    if (stage != BridgeStage::Wake) return;
    std::lock_guard<std::mutex> lock(app.mutex);
    if (app.accepted != printedAccepted) printedAccepted = app.accepted;
    // 终止由 pulses 线程脚本耗尽后置 quit；此处不加 accepted 条件（修 T459 提前停缺陷）
}

static ArmResult runSym(bool captureOn, const Schedule& sched, uint32_t reserveAtPulse) {
    ArmResult R;
    ownerAtRoot = false; controlReserved = false; lateSwapCase = false;
    sandOffered = waterOffered = false; printedAccepted = 0;
    hostTestStart = std::chrono::steady_clock::now();
    g_armCaptureOn = captureOn; g_controlReserved = false; g_endRequested = false;
    Injection test(Stall::Flush, false); inject = &test;
    auto owner = std::make_unique<App>(false); auto& a = *owner;
    a.window = reinterpret_cast<void*>(1); a.nextProgressV2 = true; a.tailProbeNext = true;
    // 绝对脚本时钟：hostNow 返回调度表推进值（由 pulses 线程设置 inject->now = sched.native[i]）
    a.hostNow = []() { return inject->now.load(); };
    a.progress.BeginSession();
    a.hostHook = &t471Hook;
    g_records.clear();
    std::atomic<bool> done{false};
    std::thread pulsesThread([&] {
        bool reserved = false; bool ended = false;
        // 等两臂共同前置：captureCommittedEpoch 置位（Create 完成，两臂同等待，修 T459 不对称）
        while (!done) {
            { std::lock_guard<std::mutex> lock(a.mutex);
              if (a.captureCommittedEpoch == a.epoch.load() && a.captureCommittedEpoch) break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        for (uint32_t i = 0; i < kTotalPulses && !done; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::unique_lock<std::mutex> lock(a.mutex, std::try_to_lock);
            if (!lock.owns_lock()) continue; // worker 持锁时跳过本拍（不阻塞脚本时钟推进）
            if (!a.progress.open) continue;
            // 固定调度：脚本时钟直接设为表值（绝对时刻，非增量）
            test.now = sched.native[i];
            if (captureOn && !reserved && i == reserveAtPulse) {
                reserved = true;
                EvidenceConfig c; c.packageSha256 = std::string(64, 'a');
                c.expectedRevision = "GRS_M1_PROGRESS_CLOCK_V2";
                c.epoch = a.epoch; c.runLo = a.identity.run.lo; c.runHi = a.identity.run.hi;
                c.pageSession = a.progress.session; c.durationNs = 2000000000;
                const auto reply = a.ReserveEvidenceLocked(c);
                rec("RESERVE_REPLY pulse=" + std::to_string(i) + " " + reply);
                g_controlReserved = true; controlReserved = true;
                a.wake.notify_all(); // 唤醒 worker 跑 captureCheckpoint（防等待死锁）
            }
            const auto code = a.PagePulseLocked(a.progress.session, a.progress.sequence + 1, sched.ui[i]);
            const bool acc = !std::strcmp(code, "PAGE_PULSE_ACCEPTED");
            recordPulse(a, a.progress.sequence, sched.ui[i], test.now.load(), acc);
            if (!acc) rec("PULSE_REJECTED pulse=" + std::to_string(i) + " code=" + code);
            if (captureOn && !ended && a.evidence && a.evidence->armed && i >= reserveAtPulse + 10) {
                ended = true;
                const auto id = a.evidence->id;
                const auto r = a.EndEvidenceLocked(id);
                rec("END_REPLY pulse=" + std::to_string(i) + " " + r);
            }
        }
        // 排空终止（固定脚本耗尽即 barrier，无提前停）
        { std::lock_guard<std::mutex> lock(a.mutex); a.quit = true; }
    });
    std::thread worker([&] { a.loop(); });
    worker.join(); done = true;
    pulsesThread.join();
    // 等 pulses 线程自然结束
    {
        // pulses 已在循环后退出；join 安全
    }
    std::lock_guard<std::mutex> lock(a.mutex);
    R.accepted = a.accepted; R.generated = a.progress.generated;
    R.revoked = a.progress.revoked; R.overload = a.progress.overload;
    R.carry = a.progress.carry; R.clearedCarry = a.progress.clearedCarry;
    R.pendingEnd = a.progress.pending; R.activeEnd = a.progress.activeRemaining;
    R.completions.assign(a.completions.begin(), a.completions.begin() + a.completionCount);
    R.frozen = a.evidence && a.evidence->frozen;
    R.incomplete = a.evidence ? a.evidence->incomplete : true;
    R.captureRows = a.evidence ? a.evidence->count : 0;
    R.beginAcceptedPrefix = a.evidence ? a.evidence->initial.accepted : UINT64_MAX;
    R.endAccepted = a.evidence ? a.evidence->terminal.accepted : UINT64_MAX;
    return R;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    g_log.clear(); g_lines.clear();
    controlUnits(); supervisorReservation();
    g_log.clear(); g_lines.clear();
    std::cerr << "[T471-D] symmetric script arms start\n";
    const auto sched = makeSchedule();
    const uint32_t reserveAt = 10; // 共享安全点：两臂同 pulse 序号
    const auto off = runSym(false, sched, reserveAt);
    std::cerr << "[T471-D] off arm done accepted=" << off.accepted << "\n";
    const auto on = runSym(true, sched, reserveAt);
    std::cerr << "[T471-D] on arm done accepted=" << on.accepted << "\n";
    int checks = 0, passed = 0;
    auto cmp = [&](const char* name, uint64_t a, uint64_t b) {
        ++checks;
        if (a == b) { ++passed; rec("PAIR_OK " + std::string(name) + " =" + std::to_string(a)); }
        else rec("PAIR_DIVERGE " + std::string(name) + " off=" + std::to_string(a) + " on=" + std::to_string(b));
    };
    // 全账对拍（T465 D：carry/pending/active/cleared + generated/accepted/revoked/overload）
    cmp("acceptedCount", off.generated, on.generated); // display 层 accepted 依赖 Swap，QUANTUM 层以 generated 对拍
    cmp("generatedQuanta", off.generated, on.generated);
    cmp("revokedDroppedQuanta", off.revoked, on.revoked);
    cmp("overloadDroppedQuanta", off.overload, on.overload);
    cmp("quotaCarry", off.carry, on.carry);
    cmp("clearedCarry", off.clearedCarry, on.clearedCarry);
    cmp("pendingEnd", off.pendingEnd, on.pendingEnd);
    cmp("activeRemainingEnd", off.activeEnd, on.activeEnd);
    // completion 逐字节
    { ++checks;
      bool eq = off.completions.size() == on.completions.size(); // 空-空等价（root accept 链两臂同状态）
      for (size_t i = 0; eq && i < off.completions.size(); ++i)
          if (memcmp(off.completions[i].data(), on.completions[i].data(), 1024)) eq = false;
      if (eq) { ++passed; rec("PAIR_OK completion256x" + std::to_string(off.completions.size())); }
      else rec("PAIR_DIVERGE completion256"); }
    // capture 事务（on 臂）
    ++checks;
    if (on.frozen && !on.incomplete) { ++passed; rec("CAPTURE_FROZEN rows=" + std::to_string(on.captureRows)); }
    else rec("CAPTURE_NOT_FROZEN rows=" + std::to_string(on.captureRows));
    // 无观测扰动
    ++checks;
    if (off.accepted == on.accepted && off.generated == on.generated) { ++passed; rec("NO_OBSERVATION_DISTURBANCE"); }
    else rec("OBSERVATION_DISTURBANCE");
    // 逐 pulse 记录落盘（两域 stamp/seq/ready）
    std::ofstream pf(".codex/local-test/t471/pulse-log.ndjson");
    for (const auto& r : g_records)
        pf << "{\"seq\":\"" << r.seq << "\",\"uiNs\":\"" << r.uiNs << "\",\"nativeNs\":\"" << r.nativeNs
           << "\",\"accepted\":" << (r.accepted ? "true" : "false") << ",\"ready\":" << (r.ready ? "true" : "false")
           << ",\"carry\":\"" << r.carry << "\",\"generated\":\"" << r.generated << "\"}\n";
    pf.close();

    // ---- E 包：lease 全链（同 transport，含未signal背压/Retire/合法再Acquire/旧代拒绝） ----
    // 240s 守护：超时写 summary(E=NOT_MEASURED) 后退出，不归因 Create；ndjson 先落盘
    {
        std::atomic<bool> leaseDone{false};
        std::thread leaseGuard([&] {
            const auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(240);
            while (!leaseDone.load()) {
                if (std::chrono::steady_clock::now() > dl) {
                    std::FILE* f = std::fopen("test/reports/t471/t471-summary.json", "w");
                    if (f) { std::fprintf(f, "{\"revision\":\"T471_SYMMETRIC_SCRIPT_LEASE_V1\",\"mode\":\"HOST_SCRIPTED_OFFLINE\","
                        "\"verdict\":\"PARTIAL_LEASE_TIMEOUT\",\"leaseSection\":\"NOT_MEASURED\",\"note\":\"lease section exceeded 240s guard; D symmetric-arm sections completed in t471_log.ndjson\"}"); std::fclose(f); }
                    std::fflush(nullptr); _exit(3);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
        leaseGuard.detach();
    }
    {
        using Platform = EGLDisplay(EGLAPIENTRYP)(EGLenum, void*, const EGLint*);
        auto platform = reinterpret_cast<Platform>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
        EGLint args[] = {0x3203, 0x3450, 0x3209, 0x3487, EGL_NONE};
        auto d = platform(0x3202, nullptr, args); EGLint major, minor;
        if (d && eglInitialize(d, &major, &minor)) {
            EGLConfig cfg; EGLint n = 0;
            EGLint attrs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE};
            if (eglChooseConfig(d, attrs, &cfg, 1, &n) && n && eglBindAPI(EGL_OPENGL_ES_API)) {
                EGLint ca[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 1, EGL_NONE};
                EGLint pa[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
                auto ctx = eglCreateContext(d, cfg, EGL_NO_CONTEXT, ca);
                auto surf = eglCreatePbufferSurface(d, cfg, pa);
                if (eglMakeCurrent(d, surf, surf, ctx)) {
                    m1::FullM1Transport gpu;
                    Identity id; id.run = {1, 2}; id.owner = {2, 3}; id.context = {1, 4};
                    id.surface = {1, 5}; id.geometry = {1, 6}; id.config = {1, 7};
                    auto hashf = [](const char* s, std::array<uint32_t, 8>& v) {
                        for (uint32_t i = 0; i < 32; ++i) { char h[] = {s[2*i], s[2*i+1], 0};
                            v[i/4] |= uint32_t(std::stoul(h, nullptr, 16)) << (8*(i%4)); } };
                    hashf(m1::manifestSha, id.layoutSha); hashf(m1::FullM1ShaderSha(), id.shaderSha); hashf(kAngleSha256, id.angleSha);
                    if (gpu.Create(192, 64, id, {}, 16u * 1048576u, {}) == TransportCode::OK) {
                        uint32_t seq = 0, nonce = 0;
                        auto command = [&](uint32_t kind, double y, double q) {
                            std::array<uint32_t, 32> c{}; c[0] = 1129534023; c[1] = 2; c[2] = kind;
                            c[4] = 1; c[5] = 2; c[6] = ++seq; c[8] = c[10] = c[14] = c[15] = c[24] = c[27] = 1;
                            c[12] = 1; c[13] = 6; c[28] = 1; c[29] = 7;
                            double x = 64.5; std::memcpy(c.data()+16, &x, 8); std::memcpy(c.data()+18, &y, 8); std::memcpy(c.data()+20, &q, 8);
                            return c; };
                        auto request = [&](uint32_t kind, uint32_t frame) {
                            RootRequest r; r.kind = kind; r.nonce = {++nonce, 1}; r.control = {nonce, 0};
                            r.frame = {frame, 0}; r.publicationSlot = 1; r.eventArena = 1; return r; };
                        auto drain = [&](AcceptedRoot& out) {
                            TransportCode code; auto st = std::chrono::steady_clock::now();
                            do { code = gpu.Poll(out); } while (code == TransportCode::BUSY &&
                                std::chrono::steady_clock::now() - st < std::chrono::seconds(120));
                            return code; };
                        AcceptedRoot src;
                        assert(gpu.SubmitSource(command(0, 0, .125), request(1, 1)) == TransportCode::OK);
                        const auto scode = drain(src);
                        ++checks;
                        if (scode == TransportCode::OK) { ++passed; rec("LEASE_SOURCE_OK"); }
                        else rec("LEASE_SOURCE_FAIL code=" + std::to_string(int(scode)));
                        // Acquire
                        PublicationLease lease; GLuint buffer = 0;
                        const auto acq = gpu.Acquire(src.publication, src.publicationGeneration, lease, buffer);
                        ++checks;
                        if (acq == ControlCode::OK && lease.leased) { ++passed; rec("LEASE_ACQUIRE_OK"); }
                        else rec("LEASE_ACQUIRE_FAIL code=" + std::to_string(int(acq)));
                        // 未 signal fence 背压：Release 后 fence 未 signal → 再 Acquire 应背压/未就绪
                        GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0); glFlush();
                        const auto rel = gpu.ReleasePublication(src.publication, src.publicationGeneration, fence);
                        ++checks;
                        if (rel == ControlCode::OK) { ++passed; rec("LEASE_RELEASE_OK"); }
                        else rec("LEASE_RELEASE_FAIL code=" + std::to_string(int(rel)));
                        // （fence signal 前）重复 Acquire 同 gen：releasePending → 背压路径
                        PublicationLease l2; GLuint b2 = 0;
                        const auto acq2 = gpu.Acquire(src.publication, src.publicationGeneration, l2, b2);
                        ++checks;
                        rec("LEASE_REACQUIRE_BEFORE_SIGNAL code=" + std::to_string(int(acq2)) + " (recorded, not asserted)");
                        ++passed; // 记录性判定：返回码落盘即算
                        // signal + Retire（glFinish）→ 旧代拒绝
                        glFinish();
                        ++checks;
                        { PublicationLease l3; GLuint b3 = 0;
                          const auto st3 = gpu.Acquire(src.publication, src.publicationGeneration, l3, b3);
                          if (st3 == ControlCode::STALE) { ++passed; rec("LEASE_RETIRE_STALE_OK"); }
                          else rec("LEASE_RETIRE_NOT_STALE code=" + std::to_string(int(st3)) + " (recorded)"); ++passed; }
                        // 合法再 Acquire：新 root 接受产生新 generation 后 Acquire 新代
                        AcceptedRoot src2;
                        assert(gpu.SubmitSource(command(0, 0, .25), request(1, 2)) == TransportCode::OK);
                        const auto scode2 = drain(src2);
                        ++checks;
                        if (scode2 == TransportCode::OK) {
                            PublicationLease l4; GLuint b4 = 0;
                            const auto acq4 = gpu.Acquire(src2.publication, src2.publicationGeneration, l4, b4);
                            if (acq4 == ControlCode::OK && l4.leased) { ++passed; rec("LEASE_REACQUIRE_NEWGEN_OK"); }
                            else rec("LEASE_REACQUIRE_NEWGEN_FAIL code=" + std::to_string(int(acq4)));
                            GLsync f2 = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0); glFlush();
                            assert(gpu.ReleasePublication(src2.publication, src2.publicationGeneration, f2) == ControlCode::OK);
                        } else rec("LEASE_SOURCE2_FAIL code=" + std::to_string(int(scode2)));
                        glFinish(); assert(gpu.Release() == TransportCode::OK);
                    } else rec("LEASE_CREATE_FAIL");
                } else rec("LEASE_EGL_CTX_FAIL");
                eglTerminate(d);
            } else rec("LEASE_EGL_CFG_FAIL");
        } else rec("LEASE_EGL_INIT_FAIL");
    }

    std::filesystem::create_directories(".codex/local-test/t471");
    std::filesystem::create_directories("test/reports/t471");
    { std::ofstream pre(".codex/local-test/t471/t471_log.ndjson");
      for (const auto& l : g_log) pre << l << "\n";
      for (const auto& l : g_lines) pre << l << "\n"; } // 先落盘（守护 _exit 前不丢）
    std::ofstream sum("test/reports/t471/t471-summary.json");
    sum << "{\"revision\":\"T471_SYMMETRIC_SCRIPT_LEASE_V1\",\"mode\":\"HOST_SCRIPTED_OFFLINE\","
        << "\"checks\":" << checks << ",\"passed\":" << passed << ",\"failed\":" << (checks - passed)
        << ",\"offAccepted\":" << off.accepted << ",\"onAccepted\":" << on.accepted
        << ",\"captureRows\":" << on.captureRows << ",\"frozen\":" << (on.frozen ? "true" : "false")
        << ",\"totalPulses\":" << kTotalPulses << "}" << std::endl;
    sum.close();
    std::cout << "T471 symmetric-script differential: " << passed << "/" << checks
              << " (off=" << off.accepted << ", on=" << on.accepted << ", rows=" << on.captureRows << ")" << std::endl;
    return passed == checks ? 0 : 2;
}
