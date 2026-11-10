// T470-C · M2 V3 故障前缀与五缺口补采 host（测试域，不改产品）。
// 复用 T452 source_host 的 SourceFixture/生产 GLSL/RootGate，新增：
//   1. 故障 completion 原件落盘（23-26/business reject/generation 溢出）
//   2. pre/post accepted bank + header + publication + dedup 原字节快照
//   3. WORK control 段（含 originU64）读回
//   4. SOURCE 成功 → QUANTUM 失败 保留 SOURCE 断言
// 全部落盘 .codex/local-test/t470-c/*.bin，独立判读在 test/t470/c_audit.py。
#include "m2/buffer_plan.h"
#include "m2/root_gate.h"
#include <memory>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "angles_generated.h"
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
using namespace grs;
using namespace grs::m2;
#include "../t452/merge_probe.inc"

static const char* OUT_DIR = ".codex/local-test/t470-c";
static unsigned g_dumpSerial = 0;

static void dumpBuf(GLuint buf, size_t bytes, const char* name) {
    std::vector<uint32_t> w(bytes / 4);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
    auto* p = reinterpret_cast<uint32_t*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, GLsizeiptr(bytes), GL_MAP_READ_BIT));
    if (!p) { std::cerr << "MAP_FAIL " << name << std::endl; return; }
    std::memcpy(w.data(), p, bytes);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    std::ofstream f(std::string(OUT_DIR) + "/" + name, std::ios::binary);
    f.write(reinterpret_cast<const char*>(w.data()), std::streamsize(bytes));
}

struct SourceFixture {
    const Profile& p; GLuint program = 0, buffers[9]{}; unsigned nonce = 0; bool bank = false;
    std::vector<uint32_t> words, work;
    std::unique_ptr<RootGate> owner;
    double maxGpuRootMs = 0;
    SourceFixture(const Profile& profile, GLuint prog, uint32_t nextSand = 1, bool poisonedGeneration = false)
        : p(profile), program(prog), words(p.stateWords), work(p.sourceBytes / 4) {
        auto* h = words.data() + p.stateWords - 256;
        h[0] = 1213420103; h[1] = 3; h[2] = 256; h[4] = h[6] = h[8] = h[10] = h[20] = h[22] = 1;
        h[24] = h[25] = h[26] = 1; h[27] = 2; h[28] = h[29] = h[30] = 3; h[31] = 7; h[39] = h[40] = 1; h[43] = p.k; h[44] = p.kw;
        h[84] = 1; h[122] = h[124] = h[125] = 1; h[123] = 65536; h[132] = 1; h[133] = h[134] = 122333; h[135] = 65536; h[136] = p.k; h[140] = 128;
        h[39] = nextSand;
        if (poisonedGeneration) words[10 * p.k + 1] = 0xffffffffu;
        Identity id{}; id.run = id.owner = id.context = id.surface = id.geometry = id.config = {1, 0};
        for (unsigned i = 0; i < 32; ++i) {
            auto hex = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
            id.layoutSha[i / 4] |= uint32_t(16 * hex(kLayoutSha[2 * i]) + hex(kLayoutSha[2 * i + 1])) << (8 * (i % 4));
        }
        std::copy(id.layoutSha.begin(), id.layoutSha.end(), h + 104);
        Words256 cold; std::copy(h, h + 256, cold.begin()); owner = std::make_unique<RootGate>(p, id, cold);
        const auto cfg = p.config.begin;
        const uint32_t frozen[] = {1, 122333, 122333, 244666, 65536, p.k, p.k, p.kw, p.pad, 4096, 128, 2 * p.k + 1,
            3, 3, 3, 3, 3, 7, 0, 1, 1, 1, 2, 1, 1, 1};
        std::copy(std::begin(frozen), std::end(frozen), work.begin() + cfg);
        glGenBuffers(9, buffers);
        alloc(0, p.stateWords * 4, words.data()); alloc(1, p.stateWords * 4, nullptr); alloc(2, p.sourceBytes, work.data());
        alloc(3, p.arenaBytes, nullptr); alloc(4, p.pubWords * 4, nullptr); alloc(5, 1024, nullptr);
    }
    ~SourceFixture() { glDeleteBuffers(9, buffers); }
    void alloc(unsigned at, size_t bytes, const void* data) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[at]); glBufferData(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(bytes), data, GL_DYNAMIC_COPY); assert(glGetError() == 0);
    }
    void edit(unsigned b, uint32_t word, uint32_t value) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[b]); glBufferSubData(GL_SHADER_STORAGE_BUFFER, word * 4, 4, &value);
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }
    std::vector<uint32_t> read(unsigned b, uint32_t count) {
        std::vector<uint32_t> out(count);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[b]);
        auto* data = reinterpret_cast<uint32_t*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, GLsizeiptr(count * 4), GL_MAP_READ_BIT)); assert(data);
        std::memcpy(out.data(), data, count * 4); assert(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER)); return out;
    }
    // 快照：accepted bank + header 区 + publication（C 包 pre/post 原字节）+ WORK control/originU64 段
    void snapshot(const char* tag) {
        ++g_dumpSerial;
        char n[160];
        std::snprintf(n, sizeof n, "%02u-%s-bank.bin", g_dumpSerial, tag);
        dumpBuf(buffers[bank], p.stateWords * 4, n);
        std::snprintf(n, sizeof n, "%02u-%s-pub.bin", g_dumpSerial, tag);
        dumpBuf(buffers[4], p.pubWords * 4, n);
        // WORK control + sourceOrigins 段按 profile WordRange 精确读回
        const auto ctrlBytes = (p.control.end - p.control.begin) * 4;
        std::snprintf(n, sizeof n, "%02u-%s-work-control.bin", g_dumpSerial, tag);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[2]);
        auto* cp = reinterpret_cast<uint32_t*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(p.control.begin * 4), GLsizeiptr(ctrlBytes), GL_MAP_READ_BIT));
        if (cp) {
            std::ofstream f(std::string(OUT_DIR) + "/" + n, std::ios::binary);
            f.write(reinterpret_cast<const char*>(cp), std::streamsize(ctrlBytes));
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
        const auto orgBytes = (p.sourceOrigins.end - p.sourceOrigins.begin) * 4;
        std::snprintf(n, sizeof n, "%02u-%s-work-origins.bin", g_dumpSerial, tag);
        auto* op = reinterpret_cast<uint32_t*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(p.sourceOrigins.begin * 4), GLsizeiptr(orgBytes), GL_MAP_READ_BIT));
        if (op) {
            std::ofstream f(std::string(OUT_DIR) + "/" + n, std::ios::binary);
            f.write(reinterpret_cast<const char*>(op), std::streamsize(orgBytes));
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
    void begin(bool source, uint32_t capacity) {
        RootRequest r{}; r.kind = source ? 1 : 2; r.quantumOrdinal = source ? 0 : 1; r.nonce = r.control = r.frame = {nonce, 0};
        r.eventCapacity = capacity; r.eventArena = nonce % 2; r.publicationSlot = !bank;
        r.dispatchCount = source ? 13 : p.quantumDispatches; r.barrierCount = r.dispatchCount + (source ? 1 : 0);
        assert(owner->Begin(r));
    }
    std::vector<uint32_t> complete(bool accept, std::chrono::steady_clock::time_point start) {
        auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0); assert(fence);
        glFlush(); GLenum state;
        do { state = glClientWaitSync(fence, 0, 1000000); } while (state == GL_TIMEOUT_EXPIRED && std::chrono::steady_clock::now() - start < std::chrono::minutes(4));
        assert(state == GL_ALREADY_SIGNALED || state == GL_CONDITION_SATISFIED); glDeleteSync(fence);
        maxGpuRootMs = std::max(maxGpuRootMs, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
        auto c = read(5, 256); AcceptedRoot out;
        if (accept) {
            auto poisoned = c; poisoned[169] ^= 128;
            assert(!owner->Inspect(poisoned.data(), poisoned.size(), 0, out));
            if (!owner->Commit(c.data(), c.size(), 0, out)) std::cerr << "M2 short gate rejected kind=" << c[27] << " outcome=" << c[28] << " nonce=" << nonce << std::endl;
            assert(owner->Bank() == uint32_t(!bank)); bank = owner->Bank() != 0;
        } else { assert(!owner->Commit(c.data(), c.size(), 0, out) && c[28] == 4); owner->Discard(); }
        return c;
    }
    std::vector<uint32_t> root(uint32_t kind, double x, double y, double q, uint32_t seq, bool accept = true, uint32_t corrupt = 0) {
        uint32_t w[32]{}; w[0] = 1129534023; w[1] = 3; w[2] = kind; w[4] = w[8] = w[10] = w[12] = w[14] = w[15] = w[24] = w[28] = 1; w[6] = seq;
        std::memcpy(w + 16, &x, 8); std::memcpy(w + 18, &y, 8); std::memcpy(w + 20, &q, 8);
        glUseProgram(program);
        auto uniform = [&](const char* name, uint32_t v) { glUniform1ui(glGetUniformLocation(program, name), v); };
        auto two = [&](const char* name, uint32_t v) { glUniform2ui(glGetUniformLocation(program, name), v, 0); };
        ++nonce; begin(true, p.sourceRows); two("uNonce", nonce); two("uControl", nonce); two("uFrame", nonce); uniform("uK", p.k); uniform("uKw", p.kw);
        uniform("uEmitQ", 16 * 1048576); uniform("uBaseBank", bank); uniform("uPublicationSlot", !bank); uniform("uEventArena", nonce % 2);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[2]); glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(w), w);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[bank]); glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[!bank]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buffers[2]); glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers[3]);
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
        const auto started = std::chrono::steady_clock::now();
        for (uint32_t pass = 0; pass < 13; ++pass) {
            if (pass == 6) {
                if (corrupt == 1) edit(3, 100, 1);
                if (corrupt == 2) edit(3, 2, MERGE);
                if (corrupt == 3) edit(3, 64, 77);
                if (corrupt == 4) edit(!bank, p.stateWords - 256 + 142, 0xffffffffu);
            }
            if (pass == 10) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers[4]);
            if (pass == 12) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers[5]);
            uniform("uPass", pass);
            glDispatchCompute(pass == 0 || pass == 6 ? p.stateWords / 64 : pass == 2 ? p.pad / 64 : 1, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT); assert(glGetError() == GL_NO_ERROR);
        }
        return complete(accept, started);
    }
    std::vector<uint32_t> quantum(GLuint program) {
        ++nonce;
        const uint32_t sandIndex = 32, waterIndex = sandIndex + p.pad, arrival = waterIndex + p.kw;
        const uint32_t edges = arrival + p.kw, frozen = edges + 127 * 16, rows = frozen + 512, angles = rows + p.k + p.kw;
        const uint32_t source = angles + 61444, spare = source + 4096 * 16, replay = spare + p.stateWords;
        const uint32_t cap = 4 * (48 * p.k + 16 * p.kw + 254) + p.k;
        begin(false, cap);
        alloc(6, p.stateWords * 4, nullptr); alloc(7, p.quantumBytes, nullptr); alloc(8, cap * 256, nullptr);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[7]); glBufferSubData(GL_SHADER_STORAGE_BUFFER, angles * 4, sizeof(grs::kAngleWords), grs::kAngleWords);
        const auto h = read(bank, p.stateWords);
        auto copy = reinterpret_cast<PFNGLCOPYBUFFERSUBDATAPROC>(eglGetProcAddress("glCopyBufferSubData")); assert(copy);
        glBindBuffer(GL_COPY_READ_BUFFER, buffers[2]); glBindBuffer(GL_COPY_WRITE_BUFFER, buffers[7]);
        const auto count = h[p.stateWords - 256 + 34]; if (count) copy(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, p.sourceLedger.begin * 4, source * 4, count * 64);
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
        glUseProgram(program);
        auto set = [&](const char* name, uint32_t v) { glUniform1ui(glGetUniformLocation(program, name), v); };
        auto two = [&](const char* name, uint32_t v) { glUniform2ui(glGetUniformLocation(program, name), v, 0); };
        two("uNonce", nonce); two("uFrame", nonce); two("uControl", nonce);
        const std::pair<const char*, uint32_t> uniforms[] = {
            {"uK", p.k}, {"uKw", p.kw}, {"uPad", p.pad}, {"uWater", 16 * p.k}, {"uColumn", 16 * (p.k + p.kw)},
            {"uHeader", p.stateWords - 256}, {"uWords", p.stateWords}, {"uSandIndex", sandIndex}, {"uWaterIndex", waterIndex}, {"uArrivalIndex", arrival},
            {"uEdges", edges}, {"uFrozen", frozen}, {"uRows", rows}, {"uAngles", angles}, {"uSourceEvents", source}, {"uEventStart", 0}, {"uEventCapacity", cap},
            {"uGravity", 512 * 1048576}, {"uSettle", 8 * 1048576}, {"uKAir", 0}, {"uKFlight", 0}, {"uKWet", 24}, {"uKappa", 24}, {"uSoft", 557537}, {"uFlux", 24 * 1048576},
            {"uQuantumOrdinal", 1}, {"uSpare", spare}, {"uReplayFrozen", replay}, {"uBaseBank", bank}, {"uPublicationSlot", !bank}, {"uEventArena", nonce % 2},
            {"uActualDispatches", p.quantumDispatches}};
        for (auto [k, v] : uniforms) set(k, v);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[bank]); glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[6]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buffers[7]); glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers[8]);
        uint32_t dispatches = 0;
        auto pass = [&](uint32_t stage, uint32_t n) { set("uPass", stage); glDispatchCompute((n + 63) / 64, 1, 1); glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT); assert(glGetError() == 0); ++dispatches; };
        auto sort = [&](uint32_t stage, uint32_t n) { for (uint32_t k = 2; k <= n; k *= 2) for (uint32_t j = k / 2; j; j /= 2) { set("uSortK", k); set("uSortJ", j); pass(stage, n); } };
        const auto started = std::chrono::steady_clock::now();
        pass(0, p.stateWords); pass(1, 1);
        for (uint32_t c = 0; c < 2; ++c) { set("uColor", c); pass(2, 127); pass(3, 1); pass(4, 127); }
        pass(5, 128); pass(6, p.pad); sort(7, p.pad); pass(8, p.kw); sort(9, p.kw); pass(10, 1); pass(11, 1);
        for (uint32_t c = 0; c < 3; ++c) { set("uColor", c); pass(12, 127); pass(13, 1); pass(14, 127); }
        pass(15, 1); pass(16, p.kw); sort(17, p.kw); pass(18, 1); pass(19, 1); pass(20, p.stateWords);
        pass(21, 1); pass(22, p.stateWords); pass(23, 1); pass(24, 1);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers[!bank]); pass(28, p.stateWords);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers[4]); pass(25, 1); pass(26, 1);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers[5]); pass(27, 1);
        assert(dispatches == p.quantumDispatches);
        return complete(true, started);
    }
};

static std::vector<uint32_t> CompileFail;

int main() {
    std::filesystem::create_directories(OUT_DIR);
    using Platform = EGLDisplay(EGLAPIENTRYP)(EGLenum, void*, const EGLint*);
    auto platform = reinterpret_cast<Platform>(eglGetProcAddress("eglGetPlatformDisplayEXT")); assert(platform);
    EGLint args[] = {0x3203, 0x3450, 0x3209, 0x3487, EGL_NONE};
    auto display = platform(0x3202, nullptr, args); assert(eglInitialize(display, nullptr, nullptr));
    EGLint attrs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE};
    EGLConfig cfg; EGLint n; assert(eglChooseConfig(display, attrs, &cfg, 1, &n) && n);
    assert(eglBindAPI(EGL_OPENGL_ES_API));
    EGLint ca[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 1, EGL_NONE}, sa[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    auto context = eglCreateContext(display, cfg, EGL_NO_CONTEXT, ca); auto surface = eglCreatePbufferSurface(display, cfg, sa);
    assert(eglMakeCurrent(display, surface, surface, context));
    auto program = CompileProbeShader(ReadShader(".codex/local-test/t452-host/source-v3.glsl"));
    auto qprogram = CompileProbeShader(ReadShader(".codex/local-test/t452-host/quantum-v3.glsl"));
    const auto& p0 = kProfiles[0]; // 192/64 单档覆盖（C 包新增数学负控不要求每档穷举）
    {
        SourceFixture f(p0, program);
        // 与 T452 精确同序（保证冲突列语义一致）
        const double maxBirth = double(p0.k * 122333u) / 1048576.0;
        auto c = f.root(0, 32.5, 0, maxBirth + .125, 1); assert(c[28] == 1);
        auto replay = f.root(0, 32.5, 0, maxBirth + .125, 1); assert(replay[28] == 3);
        auto merged = f.root(0, 32.5, 0, .125, 2); assert(merged[28] == 1);
        f.snapshot("pre-wj");
        auto w = f.root(1, 32.5, 0, .125, 3); assert(w[28] == 1);
        auto j = f.root(1, 32.5, 128, .125, 4); assert(j[28] == 1);
        auto conflict = f.root(0, 32.5, 1, .125, 2); assert(conflict[28] == 2 && conflict[64] == PAYLOAD_CONFLICT && conflict[58] == j[58]);
        std::ofstream cf(std::string(OUT_DIR) + "/reject-payload-conflict.bin", std::ios::binary);
        cf.write(reinterpret_cast<const char*>(conflict.data()), std::streamsize(conflict.size() * 4));
        f.snapshot("post-conflict");
        // SOURCE 成功→Q 成功→Q poison 拒绝保留 SOURCE（Inspect 只读，不改账）
        f.snapshot("pre-q");
        auto qbad = f.quantum(qprogram); assert(qbad[27] == 2 && qbad[28] == 1);
        f.snapshot("post-q-ok");
        {
            const auto before = f.read(f.bank, p0.stateWords);
            auto poison = qbad; poison[169] ^= 128; poison[101] = 999;
            AcceptedRoot out;
            const bool rejected = !f.owner->Inspect(poison.data(), poison.size(), 0, out);
            f.snapshot("post-q-poison-reject");
            std::ofstream pf(std::string(OUT_DIR) + "/q-poison-reject.bin", std::ios::binary);
            pf.write(reinterpret_cast<const char*>(poison.data()), std::streamsize(poison.size() * 4));
            const auto after = f.read(f.bank, p0.stateWords);
            assert(rejected && after == before); // SOURCE/账保留
        }
        // 故障 23-26 原件（corrupt 1-4）
        uint32_t modes[4] = {1, 2, 3, 4};
        const char* names[4] = {"fatal-schema-23", "fatal-order-24", "fatal-prefix-26", "fatal-mutation-25"};
        for (int m = 0; m < 4; ++m) {
            const auto before = f.read(f.bank, p0.stateWords);
            f.snapshot(modes[m] == 1 ? "pre-schema" : modes[m] == 2 ? "pre-order" : modes[m] == 3 ? "pre-prefix" : "pre-mutation");
            auto bad = f.root(1, 32.5, 128, .125, 5, false, modes[m]);
            std::ofstream bf(std::string(OUT_DIR) + "/" + names[m] + ".bin", std::ios::binary);
            bf.write(reinterpret_cast<const char*>(bad.data()), std::streamsize(bad.size() * 4));
            f.snapshot(names[m]);
            const auto after = f.read(f.bank, p0.stateWords);
            // 整 root 拒绝：bank 不变
            assert(before == after);
        }
        // generation 溢出
        {
            SourceFixture g(p0, program, 1, true);
            const auto initial = g.read(0, p0.stateWords);
            auto failure = g.root(0, 32.5, 0, .25, 1, false);
            std::ofstream gf(std::string(OUT_DIR) + "/fatal-generation-21.bin", std::ios::binary);
            gf.write(reinterpret_cast<const char*>(failure.data()), std::streamsize(failure.size() * 4));
            g.snapshot("generation");
            assert(failure[29] == 21 && g.read(0, p0.stateWords) == initial);
        }
        std::cout << "T470-C roots done, dumps=" << g_dumpSerial << std::endl;
    }
    // ---- B 包：真实 GLSL merge/RNE 数学负控（独立重算输入由 python 侧 T446 oracle 消费）----
    const double maxBirth = double(p0.k * 122333u) / 1048576.0;
    {
        SourceFixture f(p0, program);
        // 场景 B1: 精确 tie pair（两粒同 y=0、同 mass → RNE tie 取商偶）
        f.snapshot("b1-base");
        auto b1 = f.root(0, 32.5, 0, .125, 1);
        assert(b1[28] == 1);
        std::ofstream f1(std::string(OUT_DIR) + "/b1-command.bin", std::ios::binary);
        uint32_t cmd[32]{}; cmd[0] = 1129534023; cmd[1] = 3; cmd[2] = 0; cmd[4] = cmd[8] = cmd[10] = cmd[12] = cmd[14] = cmd[15] = cmd[24] = cmd[28] = 1; cmd[6] = 1;
        double xb = 32.5, yb = 0, qb = .125;
        std::memcpy(cmd + 16, &xb, 8); std::memcpy(cmd + 18, &yb, 8); std::memcpy(cmd + 20, &qb, 8);
        f1.write(reinterpret_cast<const char*>(cmd), sizeof(cmd));
        f.snapshot("b1-after");
        // 场景 B2: 满池 merge+birth（maxBirth 与 T452 full fixture 同参）
        SourceFixture full(p0, program);
        full.snapshot("b2-base");
        auto b2 = full.root(0, 32.5, 0, maxBirth, 1);
        assert(b2[28] == 1 && b2[63] == 1 && b2[166] == p0.k);
        std::ofstream f2(std::string(OUT_DIR) + "/b2-command.bin", std::ios::binary);
        double q2 = maxBirth; std::memcpy(cmd + 20, &q2, 8);
        f2.write(reinterpret_cast<const char*>(cmd), sizeof(cmd));
        full.snapshot("b2-after");
        // 场景 B3: NO_PAIR 前缀（满池后 y 超半径——与 T452 noPair 同参）
        auto b3 = full.root(0, 34.5, 0, maxBirth + .125, 2);
        assert(b3[28] == 1 && b3[63] == 2 && b3[64] == NO_PAIR);
        std::ofstream f3(std::string(OUT_DIR) + "/b3-command.bin", std::ios::binary);
        double x3 = 34.5, q3 = maxBirth + .125; std::memcpy(cmd + 16, &x3, 8); std::memcpy(cmd + 20, &q3, 8);
        f3.write(reinterpret_cast<const char*>(cmd), sizeof(cmd));
        full.snapshot("b3-after");
        std::cout << "T470-B merge/RNE fixtures done" << std::endl;
    }
    glDeleteProgram(qprogram); glDeleteProgram(program); eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface); eglDestroyContext(display, context); eglTerminate(display);
    return 0;
}
