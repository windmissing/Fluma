// T481 · 显式恢复生产接线独立差分。
// 消费 T467 真实生成 bridge（app_bridge.cpp + recovery467），断言全部从 T451v2/T464
// 合同独立推导，不复用开发 control_host 的 23 项自测；覆盖快速两 reset、窗口/后台取消、
// capture RUN_RESET 终态、GAP/250ms/seq 边界。HOST_SCRIPTED 离线。
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cassert>
#include <iostream>
#include <fstream>
#define GRS_APP_M1
#define GRS_BRIDGE_HOST_TEST
#include "../../.codex/local-test/t467-unit/entry/src/main/cpp/gpu_resident/app_bridge.cpp"
extern "C" void GL_APIENTRY glFinish() { auto f = reinterpret_cast<PFNGLFINISHPROC>(eglGetProcAddress("glFinish")); assert(f); f(); }
extern "C" void GL_APIENTRY glCopyBufferSubData(GLenum a, GLenum b, GLintptr c, GLintptr d, GLsizeiptr n) { auto f = reinterpret_cast<PFNGLCOPYBUFFERSUBDATAPROC>(eglGetProcAddress("glCopyBufferSubData")); assert(f); f(a, b, c, d, n); }
extern "C" void GL_APIENTRY glUniform1f(GLint a, GLfloat v) { auto f = reinterpret_cast<PFNGLUNIFORM1FPROC>(eglGetProcAddress("glUniform1f")); assert(f); f(a, v); }
extern "C" void GL_APIENTRY glUniform4fv(GLint a, GLsizei n, const GLfloat *v) { auto f = reinterpret_cast<PFNGLUNIFORM4FVPROC>(eglGetProcAddress("glUniform4fv")); assert(f); f(a, n, v); }
extern "C" void GL_APIENTRY glGetShaderInfoLog(GLuint a, GLsizei n, GLsizei *len, GLchar *out) { auto f = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(eglGetProcAddress("glGetShaderInfoLog")); assert(f); f(a, n, len, out); }
extern "C" void GL_APIENTRY glGetProgramInfoLog(GLuint a, GLsizei n, GLsizei *len, GLchar *out) { auto f = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(eglGetProcAddress("glGetProgramInfoLog")); assert(f); f(a, n, len, out); }
static uint64_t nowNs = 1000000000;
static int checks = 0, passed = 0;
static void check(const char *id, bool ok, const std::string &detail) {
  ++checks; if (ok) ++passed;
  std::cout << (ok ? "PASS " : "FAIL ") << id << " | " << detail << std::endl;
}
static Identity MakeId(uint32_t run) { Identity id{}; id.run = {run, 0}; id.owner = id.context = id.surface = id.geometry = id.config = {1, 0}; return id; }
static void Request(App &a, bool v2) { a.nextProgressV2 = v2; a.invalidate(nullptr, false); a.progressV2 = v2; }
static void Resources(App &a, uint32_t run) {
  // GL owner 显式退休两步：撤旧 → 退休完成（tombstone 登记），然后才可交接新资源。
  assert(a.recovery.Retiring(a.epoch));
  assert(a.recovery.Retired(a.epoch));
  a.progress.ResetRun(); a.progress.open = false; a.progress.hasPulse = false;
  a.transportIdentity = MakeId(run); a.identity = a.transportIdentity; a.input = {1, 0};
  a.v2ResourcesReady = a.recovery.Resources(a.epoch, a.window, a.transportIdentity);
}
int main() {
  auto a = std::make_unique<App>(false);
  a->hostNow = []() { return nowNs; };
  a->window = reinterpret_cast<void *>(1);
  a->ui.SetMode(false);

  // ---- R1 首轮恢复事务链（T464 合同：GAP→显式新空场→退休→资源→新 session→READY）----
  Request(*a, true);
  const auto serial0 = a->recovery.serial, epoch0 = a->epoch.load();
  check("R1_REQUEST_STATE", !a->progress.open && !a->AllowedLocked() &&
        a->recovery.Retiring(epoch0) && a->recovery.Retired(epoch0),
        "显式新空场后 open=false、permit 撤、 retiring/retired 登记");
  check("R1_NO_SESSION_BEFORE_HANDOFF", a->BeginPageSessionLocked() == 0,
        "资源交接前新 session 不建立");

  nowNs += 39000000000ull; // shader 编译窗口（≥GAP 域）
  Resources(*a, 1);
  a->TryReadyV2Locked();
  check("R2_RESOURCES_NOT_READY", !a->AllowedLocked() && a->progress.pending == 0,
        "资源完成后仍无 session → 未 READY（首 pulse 建 baseline 前）");

  // session 身份门：错 run / 旧 serial 拒绝
  check("R3_SESSION_IDENTITY_GATE",
        a->RecoverySessionLocked(serial0, epoch0, {999, 0}, 1) == 0 &&
            a->RecoverySessionLocked(serial0 - 1, epoch0, {1, 0}, 1) == 0,
        "错 run、旧 serial 的 session 请求拒绝");

  auto token = a->RecoverySessionLocked(serial0, epoch0, U64{1, 0}, 1);
  check("R4_SESSION_TOKEN_VALID", token != 0, "合法身份 session token 建立且非零");
  auto firstCode = std::string(a->PagePulseLocked(token, 1, nowNs));
  check("R5_FIRST_PULSE_BASELINE", firstCode == "PAGE_PULSE_ACCEPTED" &&
        a->AllowedLocked() && a->readyEpoch == epoch0 &&
        a->progress.generated == 0,
        "首 pulse 接受建 baseline：READY 达成但 baseline 拍不产 quota（generated=0）");
  nowNs += 8400000; a->PagePulseLocked(token, 2, nowNs);
  nowNs += 8400000; a->PagePulseLocked(token, 3, nowNs);
  check("R6_READY_AFTER_SECOND_PULSE", a->AllowedLocked() &&
        a->readyEpoch == epoch0 && a->progress.generated == 1 &&
        a->progress.Balanced(),
        "第二 pulse 起 quota；readyEpoch==epoch；账平衡");

  // ---- R7 GAP 撤权（V2 pulse 域 33.3ms）----
  nowNs += 33333334;
  a->ObserveControlLocked(nowNs);
  check("R7_GAP_REVOKED", !a->AllowedLocked() && !a->progress.open &&
        a->recovery.phase == recovery467::Phase::FAULTED_REVOKED,
        "GAP 撤权 → FAULTED_REVOKED、open=false");
  for (unsigned i = 0; i < 3; ++i) {
    ++nowNs;
    check(i == 0 ? "R8_STALE_PULSE" : "R8B",
          std::string(a->PagePulseLocked(token, 20 + i, nowNs)) == "STALE_PAGE_SESSION",
          "GAP 后旧 token pulse → STALE_PAGE_SESSION");
  }

  // ---- R9 快速两 reset：代际滚动 + 旧 token 无效 ----
  auto epochAtSecond = a->epoch.load();
  Request(*a, true);
  auto serial2 = a->recovery.serial;
  Request(*a, true); // 第二次立即 reset（快速两 reset）
  auto serial3 = a->recovery.serial;
  check("R9_SERIAL_ROLLS", serial3 > serial2 && serial2 > serial0,
        "每次显式新空场 resetSerial 严格递增");
  Resources(*a, 2);
  auto token2 = a->RecoverySessionLocked(serial3, a->epoch.load(), U64{2, 0}, 1);
  check("R9B_OLD_TOKEN_REJECTED",
        token2 != 0 &&
        std::string(a->PagePulseLocked(token, 99, nowNs)) == "STALE_PAGE_SESSION",
        "旧 token 在新代拒绝（STALE_PAGE_SESSION）；新代 token 非零");
  a->PagePulseLocked(token2, 1, nowNs);
  check("R9C_NEW_GEN_READY", a->AllowedLocked(), "新代 session 1 pulse 后 READY");

  // ---- R10 窗口/后台取消 ----
  a->setActive(false);
  check("R10_BACKGROUND_UNMOUNTS", !a->AllowedLocked(),
        "后台 → permit 卸下");
  check("R10B_SESSION_CANCELLED",
        a->RecoverySessionLocked(a->recovery.serial, a->epoch, {2, 0}, 2) == 0,
        "后台期间新 session 请求取消");
  a->setActive(true);
  check("R10C_RESUME_NOT_AUTO_READY", !a->AllowedLocked(),
        "回前台不自动 READY（无定时器伪心跳/旧 permit 续期）");

  // ---- R11 capture RUN_RESET 终态（RESERVED 取消 / ARMED 冻结 / reason 保持）----
  Request(*a, true);
  a->captureRequest.state = App::CaptureRequest::RESERVED;
  a->captureRequest.armByNs = UINT64_MAX;
  Request(*a, true);
  check("R11_RESERVED_CANCELLED",
        a->captureRequest.state == App::CaptureRequest::CANCELLED &&
        std::string(a->captureRequest.reason) == "RUN_RESET",
        "RESERVED 采集遇 RUN_RESET → CANCELLED/RUN_RESET");
  a->evidence = App::NewCaptureStorage();
  a->evidence->armed = true;
  const auto phaseBefore = a->recovery.phase;
  Request(*a, true);
  check("R11B_ARMED_FROZEN",
        a->evidence->frozen && !a->evidence->armed &&
        std::string(a->evidence->reason.data()) == "RUN_RESET",
        "ARMED 采集遇 RUN_RESET → 冻结（frozen=" +
            std::string(a->evidence->frozen ? "1" : "0") + " armed=" +
            std::string(a->evidence->armed ? "1" : "0") + " reason=" +
            std::string(a->evidence->reason.data()) + " phaseBefore=" +
            recovery467::Name(phaseBefore) + " phaseNow=" +
            recovery467::Name(a->recovery.phase) + "）");
  auto originalReason = a->evidence->reason;
  Request(*a, true);
  check("R11C_REASON_IMMUTABLE", a->evidence->reason == originalReason,
        "后续 reset 不改写已冻结 reason（终态一次定型）");
  a->captureRequest = App::CaptureRequest{}; // 清 capture 状态（R12 干净起点）

  // ---- R12 250ms 期限边界（同 permit 不续期；renewal 非延续）----
  Request(*a, true); Resources(*a, 7);
  auto t = a->RecoverySessionLocked(a->recovery.serial, a->epoch, U64{7, 0}, 1);
  a->PagePulseLocked(t, 1, ++nowNs);
  a->permit = {a->identity, a->input, a->epoch, a->permissionRevision,
               nowNs + 250000000, 0, a->window};
  a->permitSession = t; a->permitPlan = 1; a->progress.active.id = 1;
  a->permit.nonce = 7; a->permitKind = 2; a->permitStarted = nowNs; a->jobLive = true;
  RootRequest r{}; r.kind = 2; r.nonce = {7, 0};
  assert(a->PermitLocked(r, nowNs));
  const auto deadline = a->permit.deadline;
  a->FreezePermitLocked(a->epoch, a->window, *(new m1::RootIsolation()), 7, 2);
  check("R12_FREEZE_PRESERVES_DEADLINE", a->permit.deadline == deadline,
        "冻结不缩短/延续期限（250ms 保持）");
  uint64_t seq = 1;
  while (nowNs + 8000000 < deadline) {
    nowNs += 8000000;
    a->PagePulseLocked(t, ++seq, nowNs);
  }
  check("R12B_DEADLINE_HONORED",
        a->PermitLocked(r, deadline) && !a->PermitLocked(r, deadline + 1),
        "期限边界：恰到期接受、+1ns 拒绝（无宽限）");
  Request(*a, true);
  check("R12C_RESET_KILLS_PERMIT", !a->PermitLocked(r, nowNs),
        "reset 后旧 permit 立即失效");

  // ---- R13 旧代资源/事务拒绝（代际隔离）----
  auto oldEpoch = a->epoch.load();
  Request(*a, true);
  auto staleSerial = a->recovery.serial;
  Request(*a, true);
  check("R13_STALE_RESOURCES_REFUSED",
        !a->recovery.Resources(oldEpoch, a->window, MakeId(3)) &&
        !a->recovery.CanSession(staleSerial, oldEpoch, {2, 0}, a->window),
        "旧 epoch 资源交接与旧 serial session 双拒绝");
  Resources(*a, 4);
  a->progress.session = UINT64_MAX;
  check("R13B_SESSION_OVERFLOW_REJECTED",
        a->RecoverySessionLocked(a->recovery.serial, a->epoch, {4, 0}, 1) == 0 &&
        a->requiresReset,
        "session 溢出拒绝并置 requiresReset");

  // ---- R14 V1/V2 冷 clock 选择 ----
  a->progress.session = 100;
  Request(*a, false); Resources(*a, 5);
  auto v1 = a->RecoverySessionLocked(a->recovery.serial, a->epoch, U64{5, 0}, 1);
  a->PagePulseLocked(v1, 1, ++nowNs);
  check("R14_V1_COLD_CLOCK", a->AllowedLocked() && !a->progressV2,
        "冷选 V1 → V1 clock READY");
  Request(*a, true); Resources(*a, 6);
  auto v2 = a->RecoverySessionLocked(a->recovery.serial, a->epoch, U64{6, 0}, 1);
  a->PagePulseLocked(v2, 1, ++nowNs);
  check("R14B_V2_COLD_CLOCK", a->AllowedLocked() && a->progressV2 && v2 > v1,
        "冷选 V2 → V2 clock READY，token 递增");

  // ---- R15 事务 tombstone（pending root/lease 退休路径独立核）----
  recovery467::Transaction tx;
  check("R15_TX_REQUEST_RETIRING", tx.Request(1, (void *)1) && tx.Retiring(1),
        "事务请求 → RETIRING");
  check("R15B_RESOURCES_BLOCKED_WHILE_RETIRING",
        !tx.Resources(1, (void *)1, MakeId(1)) &&
        tx.phase == recovery467::Phase::RETIRING_OLD,
        "退休完成前资源交接拒绝");
  for (unsigned i = 0; i < 600; ++i) tx.Note({"PULSE"});
  check("R15C_RING_BOUNDED_DROPS", tx.dropped == 88,
        "512 固定事件环：600 溢出 88 条丢弃计数");
  tx.Fault("FIRST"); tx.Fault("SECOND");
  check("R15D_FIRST_FAULT_LATCHED", std::string(tx.firstReason) == "FIRST",
        "首错锁存（SECOND 不覆盖 FIRST）");

  std::ofstream out("test/reports/t481/t481-recovery-diff.json");
  out << "{\"verdict\":\"" << (passed == checks ? "PASS" : "FAIL")
      << "\",\"passed\":" << passed << ",\"total\":" << checks
      << ",\"schema\":\"T481_RECOVERY_DIFF_V1\",\"domain\":\"HOST_SCRIPTED_OFFLINE\"}";
  std::cout << "T481 recovery differential: " << passed << "/" << checks << " PASS\n";
  return passed == checks ? 0 : 1;
}
