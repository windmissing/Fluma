// T487 · T484 §3 受控 BASE 数值场景 host（测试域，不改产品）。
// 复用 T479 host 的 SourceFixture（真实接受 SOURCE 种子 + edit patch 合法位置/速度）。
// GLSL 用 T482 coherent 修复冻结版：
//   source:  .codex/local-test/t482-final/source-v3.glsl   sha b7d0a027128811db…
//   quantum: .codex/local-test/t482-final/quantum-v3.glsl  sha 0ef4f0457a8be135…
// 期望由 test/t487/scenarios_expected.py 在 GPU 执行前冻结（t487-scenarios-expected.json）；
// 本 host 逐场景：真实种子 → patch 前态 → 一次 SOURCE → 四 snapshot + completion 落盘。
// 断言判读在 test/t487/scenarios_audit.py（与 host 分离，期望不由 completion 反推）。
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

static const char* OUT_DIR = ".codex/local-test/t487-g";
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

// ==== SourceFixture 与 T479 相同（真实 RootGate/13 pass/短接受）====
struct SourceFixture {
    const Profile& p; GLuint program = 0, buffers[9]{}; unsigned nonce = 0; bool bank = false;
    std::vector<uint32_t> words, work;
    std::unique_ptr<RootGate> owner;
    double maxGpuRootMs = 0;
    SourceFixture(const Profile& profile, GLuint prog, uint32_t nextSand = 1)
        : p(profile), program(prog), words(p.stateWords), work(p.sourceBytes / 4) {
        auto* h = words.data() + p.stateWords - 256;
        h[0] = 1213420103; h[1] = 3; h[2] = 256; h[4] = h[6] = h[8] = h[10] = h[20] = h[22] = 1;
        h[24] = h[25] = h[26] = 1; h[27] = 2; h[28] = h[29] = h[30] = 3; h[31] = 7; h[39] = h[40] = 1; h[43] = p.k; h[44] = p.kw;
        h[84] = 1; h[122] = h[124] = h[125] = 1; h[123] = 65536; h[132] = 1; h[133] = h[134] = 122333; h[135] = 65536; h[136] = p.k; h[140] = 128;
        h[39] = nextSand;
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
    void snapshot(const char* tag) {
        ++g_dumpSerial;
        char n[160];
        std::snprintf(n, sizeof n, "%02u-%s-bank.bin", g_dumpSerial, tag);
        dumpBuf(buffers[bank], p.stateWords * 4, n);
        std::snprintf(n, sizeof n, "%02u-%s-pub.bin", g_dumpSerial, tag);
        dumpBuf(buffers[4], p.pubWords * 4, n);
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
            if (!owner->Commit(c.data(), c.size(), 0, out)) std::cerr << "gate rejected kind=" << c[27] << " outcome=" << c[28] << " nonce=" << nonce << std::endl;
            assert(owner->Bank() == uint32_t(!bank)); bank = owner->Bank() != 0;
        } else { assert(!owner->Commit(c.data(), c.size(), 0, out) && c[28] == 4); owner->Discard(); }
        return c;
    }
    std::vector<uint32_t> root(uint32_t kind, double x, double y, double q, uint32_t seq, bool accept = true) {
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
            if (pass == 10) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers[4]);
            if (pass == 12) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buffers[5]);
            uniform("uPass", pass);
            glDispatchCompute(pass == 0 || pass == 6 ? p.stateWords / 64 : pass == 2 ? p.pad / 64 : 1, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT); assert(glGetError() == GL_NO_ERROR);
        }
        return complete(accept, started);
    }
    // 命令字节原件（T484 §1：归档真正上传的同一份 32U32）
    void dumpCommand(uint32_t kind, double x, double y, double q, uint32_t seq, const char* name) {
        uint32_t w[32]{}; w[0] = 1129534023; w[1] = 3; w[2] = kind; w[4] = w[8] = w[10] = w[12] = w[14] = w[15] = w[24] = w[28] = 1; w[6] = seq;
        std::memcpy(w + 16, &x, 8); std::memcpy(w + 18, &y, 8); std::memcpy(w + 20, &q, 8);
        std::ofstream f(std::string(OUT_DIR) + "/" + name, std::ios::binary);
        f.write(reinterpret_cast<const char*>(w), sizeof(w));
    }
};

// SoA 字段布局（T484 §2：field*K+slot）
enum { F_MASS = 0, F_X = 1, F_Y = 2, F_VX = 3, F_VY = 4, F_ID = 5, F_GEN = 6, F_F7 = 7, F_ORGLO = 8, F_ORGHI = 9, F_TAG = 10, F_MED = 11, F_WAIT = 12, F_SID = 13 };

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
    const auto started = std::chrono::steady_clock::now();
    auto program = CompileProbeShader(ReadShader(".codex/local-test/t482-final/source-v3.glsl"));
    std::cout << "compileSec=" << std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count() << std::endl;
    const auto& p0 = kProfiles[0];

    const double R = 65536.0 * 1048576.0 / 1048576.0; // 不用：y 域直接用 Q20 整数经 double
    // Q20 定点：y_Q20 = yUnits * 65536（1R=65536 Q20 units）。命令 y 是 binary64→shader floor 到 Q20。
    // T479 用 y=0/128 等 binary64；这里场景 y 用 units*65536 的精确 binary64（2^36 内整数精确）。
    {
        // ---- S1/S2/S8 共用模式：空池出 3 粒（3 次 .125 SOURCE），patch y 再触发 pair ----
        // 首个验证场景：S1 同距离 tie。
        SourceFixture f(p0, program);
        f.snapshot("s1-base");
        const double maxBirth = double(p0.k * 122333u) / 1048576.0;
        { auto c0 = f.root(0, 32.5, 0, maxBirth, 1); assert(c0[28] == 1); }  // 满池 192 粒 y=0
        f.snapshot("s1-seeded");
        // 满池（decide 才会走 STEP_PAIR）。patch：ID1 y=2R、ID2/3 y=2R+2（与其余 y=0 粒距离 >R，
        // 合法 pair 只在 {1,2,3} 内）→ tie 期望 (1,2)。
        auto bank = f.bank;
        const uint32_t K = p0.k;
        const uint32_t y1 = 2u * 65536u, y23 = 2u * 65536u + 2u;
        f.edit(bank, F_Y * K + 0, y1);
        f.edit(bank, F_Y * K + 1, y23);
        f.edit(bank, F_Y * K + 2, y23);
        f.snapshot("s1-patched");
        f.dumpCommand(0, 32.5, 0, .125, 2, "s1-command.bin");
        auto c4 = f.root(0, 32.5, 0, .125, 2);
        { std::ofstream cf(std::string(OUT_DIR) + "/s1-completion.bin", std::ios::binary);
          cf.write(reinterpret_cast<const char*>(c4.data()), std::streamsize(c4.size() * 4)); }
        std::cout << "S1 outcome=" << c4[28] << " stop=" << c4[64] << std::endl;
        f.snapshot("s1-after");
        // patch 记录（patchWord 清单：oldValue 需读回——简化为 BASE 侧 seeded 快照可查）
        { auto fb = std::ofstream(std::string(OUT_DIR) + "/s1-patch.json"); fb << "{\"buffer\":\"bank\",\"words\":["
            << (F_Y * K + 0) << "," << (F_Y * K + 1) << "," << (F_Y * K + 2)
            << "],\"values\":[" << y1 << "," << y23 << "," << y23 << "]}" << std::endl; }

        // ---- S2: 重置新 fixture，3 粒后 patch ID1 y=2R、ID2 y=3R、ID3 y=2R+1 ----
        SourceFixture f2(p0, program);
        { auto c0 = f2.root(0, 32.5, 0, maxBirth, 1); assert(c0[28] == 1); }  // 满池
        f2.snapshot("s2-seeded");
        // ID1 y=2R、ID2 y=3R（差 R 可配）、ID3 y=2R+1（与 ID1 差 1 更近但 idB=3>2）
        f2.edit(f2.bank, F_Y * p0.k + 0, 2u * 65536u);
        f2.edit(f2.bank, F_Y * p0.k + 1, 3u * 65536u);
        f2.edit(f2.bank, F_Y * p0.k + 2, 2u * 65536u + 1u);
        f2.snapshot("s2-patched");
        f2.dumpCommand(0, 32.5, 0, .125, 2, "s2-command.bin");
        auto c2 = f2.root(0, 32.5, 0, .125, 2);
        { std::ofstream cf(std::string(OUT_DIR) + "/s2-completion.bin", std::ios::binary);
          cf.write(reinterpret_cast<const char*>(c2.data()), std::streamsize(c2.size() * 4)); }
        std::cout << "S2 outcome=" << c2[28] << " stop=" << c2[64] << std::endl;
        f2.snapshot("s2-after");

        // ---- S3: 半径三例（同一 fixture 串行：每例前先补粒保证恰好 2 粒可配）----
        // S3m1：2 粒 y 差 R−1 → 配；S3z：再 2 粒差 R → 配；S3p1：再 2 粒差 R+1 → NO_PAIR
        SourceFixture f3(p0, program);
        { auto a = f3.root(0, 32.5, 0, maxBirth, 1); assert(a[28] == 1); }  // 满池
        // m1：ID1/2 差 R−1 → 配（merge 保 id1，腾 1 槽 → 新 birth id=新序）
        f3.edit(f3.bank, F_Y * p0.k + 0, 2u * 65536u);
        f3.edit(f3.bank, F_Y * p0.k + 1, 3u * 65536u - 1u);
        f3.snapshot("s3m1-patched");
        f3.dumpCommand(0, 32.5, 0, .125, 2, "s3m1-command.bin");
        auto cm = f3.root(0, 32.5, 0, .125, 2);
        { std::ofstream cf(std::string(OUT_DIR) + "/s3m1-completion.bin", std::ios::binary);
          cf.write(reinterpret_cast<const char*>(cm.data()), std::streamsize(cm.size() * 4)); }
        std::cout << "S3m1 outcome=" << cm[28] << " stop=" << cm[64] << std::endl;
        f3.snapshot("s3m1-after");
        // z（恰好 R）：新 fixture 满池（m1 后池有空槽，decide 会走 BIRTH）
        SourceFixture f3z(p0, program);
        { auto c = f3z.root(0, 32.5, 0, maxBirth, 1); assert(c[28] == 1); }
        f3z.snapshot("s3z-seeded");
        f3z.edit(f3z.bank, F_Y * p0.k + 0, 2u * 65536u);
        f3z.edit(f3z.bank, F_Y * p0.k + 1, 3u * 65536u);
        f3z.snapshot("s3z-patched");
        f3z.dumpCommand(0, 32.5, 0, .125, 2, "s3z-command.bin");
        auto cz = f3z.root(0, 32.5, 0, .125, 2);
        { std::ofstream cf(std::string(OUT_DIR) + "/s3z-completion.bin", std::ios::binary);
          cf.write(reinterpret_cast<const char*>(cz.data()), std::streamsize(cz.size() * 4)); }
        std::cout << "S3z outcome=" << cz[28] << " stop=" << cz[64] << std::endl;
        f3z.snapshot("s3z-after");
        // p1（R+1 → NO_PAIR）：新 fixture 满池
        SourceFixture f3p(p0, program);
        { auto c = f3p.root(0, 32.5, 0, maxBirth, 1); assert(c[28] == 1); }
        f3p.snapshot("s3p1-seeded");
        f3p.edit(f3p.bank, F_Y * p0.k + 0, 2u * 65536u);
        f3p.edit(f3p.bank, F_Y * p0.k + 1, 3u * 65536u + 1u);
        f3p.snapshot("s3p1-patched");
        f3p.dumpCommand(0, 32.5, 0, .125, 2, "s3p1-command.bin");
        auto cp = f3p.root(0, 32.5, 0, .125, 2);
        { std::ofstream cf(std::string(OUT_DIR) + "/s3p1-completion.bin", std::ios::binary);
          cf.write(reinterpret_cast<const char*>(cp.data()), std::streamsize(cp.size() * 4)); }
        std::cout << "S3p1 outcome=" << cp[28] << " stop=" << cp[64] << std::endl;
        f3p.snapshot("s3p1-after");

        // ---- S4: 非 eligible heavy 夹中间（先真实 merge 出 244666 heavy 粒）----
        // 满包 emit → K=192 粒 122333；再 .125 → 首对 merge 出 244666 粒。
        SourceFixture f4(p0, program);
        { auto c = f4.root(0, 32.5, 0, maxBirth, 1); assert(c[28] == 1); }
        f4.snapshot("s4-fullpool");
        { auto c = f4.root(0, 32.5, 0, .125, 2); assert(c[28] == 1); }
        f4.snapshot("s4-merged"); // 含 1 个 244666 heavy（merge 产生）+ 190 light
        // patch：heavy 槽 y=2R+4 夹在两 light 之间（light y=2R / 2R+2）
        // heavy 槽定位：读 bank 找 mass==244666 的槽
        auto b4 = f4.read(f4.bank, p0.stateWords);
        int heavySlot = -1, l1 = -1, l2 = -1;
        for (uint32_t s = 0; s < p0.k; ++s) {
            uint32_t m = b4[F_MASS * p0.k + s];
            if (m == 244666 && heavySlot < 0) heavySlot = int(s);
            else if (m == 122333) { if (l1 < 0) l1 = int(s); else if (l2 < 0) l2 = int(s); }
        }
        std::cout << "S4 slots heavy=" << heavySlot << " l1=" << l1 << " l2=" << l2 << std::endl;
        assert(heavySlot >= 0 && l1 >= 0 && l2 >= 0);
        f4.edit(f4.bank, F_Y * p0.k + uint32_t(l1), 2u * 65536u);
        f4.edit(f4.bank, F_Y * p0.k + uint32_t(heavySlot), 2u * 65536u + 4u);
        f4.edit(f4.bank, F_Y * p0.k + uint32_t(l2), 2u * 65536u + 2u);
        f4.snapshot("s4-patched");
        f4.dumpCommand(0, 32.5, 0, .125, 3, "s4-command.bin");
        auto c4b = f4.root(0, 32.5, 0, .125, 3);
        { std::ofstream cf(std::string(OUT_DIR) + "/s4-completion.bin", std::ios::binary);
          cf.write(reinterpret_cast<const char*>(c4b.data()), std::streamsize(c4b.size() * 4)); }
        std::cout << "S4 outcome=" << c4b[28] << " stop=" << c4b[64] << std::endl;
        f4.snapshot("s4-after");

        // ---- S8: merge 后重新选对（一次 SOURCE 多 birth：满池只差 2 槽时 emit .25）----
        // 池：3 light（互不配，y 间隔 >R）+ emit .25=2 birth → 需 2 merge 腾 2 槽，
        // 每行 pair 基于变更后全池（重选语义）。
        SourceFixture f8(p0, program);
        { auto c0 = f8.root(0, 32.5, 0, maxBirth, 1); assert(c0[28] == 1); }  // 满池
        f8.snapshot("s8-seeded");
        // (1,2) y 差 2 可配；(1,3) 差 7R 不可配 → merge(1,2) 腾槽 birth1 → 池又满
        // → 重选全池无 eligible → NO_PAIR 保前缀（V3 语义；V4 all-or-none 归 T499）
        f8.edit(f8.bank, F_Y * p0.k + 0, 2u * 65536u);
        f8.edit(f8.bank, F_Y * p0.k + 1, 2u * 65536u + 2u);
        f8.edit(f8.bank, F_Y * p0.k + 2, 9u * 65536u);
        f8.snapshot("s8-patched");
        f8.dumpCommand(0, 32.5, 0, .25, 2, "s8-command.bin");
        auto c8 = f8.root(0, 32.5, 0, .25, 2);
        { std::ofstream cf(std::string(OUT_DIR) + "/s8-completion.bin", std::ios::binary);
          cf.write(reinterpret_cast<const char*>(c8.data()), std::streamsize(c8.size() * 4)); }
        std::cout << "S8 outcome=" << c8[28] << " stop=" << c8[64] << std::endl;
        f8.snapshot("s8-after");

        // ---- 入口负控（T484 §2 尾）：duplicate ID ----
        SourceFixture f9(p0, program);
        { auto c = f9.root(0, 32.5, 0, .125, 1); assert(c[28] == 1); }
        f9.snapshot("neg-dup-seeded");
        // patch 第二槽 id 与首槽相同（不合法：同代同列同 sourceId 下 id 重复）
        auto id0 = f9.read(f9.bank, p0.stateWords)[F_ID * p0.k + 0];  // word 5*K+0 = slot0 id
        f9.edit(f9.bank, F_ID * p0.k + 1, id0);
        f9.snapshot("neg-dup-patched");
        f9.dumpCommand(0, 32.5, 0, .125, 2, "neg-dup-command.bin");
        auto cn = f9.root(0, 32.5, 0, .125, 2, false);  // 审核拒 → accept=false（bank 不变）
        { std::ofstream cf(std::string(OUT_DIR) + "/neg-dup-completion.bin", std::ios::binary);
          cf.write(reinterpret_cast<const char*>(cn.data()), std::streamsize(cn.size() * 4)); }
        std::cout << "NEG-dup outcome=" << cn[28] << " stop=" << cn[64] << std::endl;
        f9.snapshot("neg-dup-after");

        std::cout << "T487 fixtures done" << std::endl;
    }
    glDeleteProgram(program); eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface); eglDestroyContext(display, context); eglTerminate(display);
    return 0;
}
