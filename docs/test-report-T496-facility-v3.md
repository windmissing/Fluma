# T496 v3 · 测试设施修复（T547 §1–3）+ 有界 attempt 复跑

日期：2026-11-17，tester，离线（ANGLE host fixture，不使用真机）。
依据：T547 归因（architecture-T547-create-and-empty-export-triage-v1.md + 机器表 manifest）。
被测设施：`test/t496/`（runner/gate/sink/harness），冻结树源 `.codex/local-test/t492-m1-delivery`。
证据：`test/reports/t496/` 与 `.codex/local-test/t496/`（v3f/v3g 原件）。

## T547 §1 点名的设施缺陷逐项修复

1. **非空 layout 门**（原 findstr "sizeof" 匹配 0 行→两空文件 fc 相等→假绿 GATE_OK）：
   新 `test/t496/layout_gate.py`——提取 `class …FullM1Transport…size(...)` 段落正文
   （1331B 双侧），空段=FAIL、段不相等=FAIL（带 diff 前 20 行）。门验证：
   `LAYOUT_SECTION_OK bytes=1331/1331 → GATE_OK`。
2. **trace 身份缺失/跨臂继承**（原 RunId/Arm 空串+固定路径 app 模式）：
   `gl_sink.inc` 增加 `t496_sink_initialize(runId,arm,attemptId)` 显式注入（一次姓 CAS，
   二次调用拒绝）；trace 路径=attemptId 唯一文件且 create-new（复用即拒绝）；每记录加
   `attemptId`+`pid` 域。验证：v3f/v3g trace 首行含完整身份域，attempt 文件互不继承。
3. **runner 边界**（原 taskkill /im 杀全部 t496.exe+窗口标题清理）：
   新 `run_create_only.sh`——watchdog 绑定本子进程 PID（`kill -0` 轮询→`kill -9` 精确），
   watchdog 收据输出 `WATCHDOG_KILL pid=… at=ISO时间`，不碰其它会话；exit124 不再翻译为
   业务断言 FAIL。run_create_only.cmd 同步保留（PowerShell Start-Process 路径在
   CokePLUS 环境下待修——bash runner 为本轮主通道）。
4. **attempt 隔离**：harness 接受 `argv[2]=attemptId`，log/stages/trace 全部按 attemptId
   唯一命名；out 目录 fresh（复用拒绝）。

## 有界 attempt 复跑（C0，900s watchdog）

- attemptId=`20261117-496v3g-c0`，CHILD_PID=499，WATCHDOG_KILL at=17:25:22+08:00（900s 到点）
- trace 330 行全配对（0 unpaired、0 JSON 坏行）——身份域+配对核全过
- 业务结果：`captureCommittedEpoch` 仍未达（与 v2 相同域：编译后 create 段/资源 commit
  握手）——本轮目标是**设施正确性**，业务卡点不变（精确切点仍需机器表全量 sink，交
  developer seam 扩展或 tester 后续装配轮）

## 审计修正（T547 §1 对 v2 报告的纠正，全部接受）

- v2 报告的 306/576 行、C2 18 次 compile、"环境 SHA 不匹配致冷重编译"结论**不被原件支持**
  （C2 文件前 378 行是 C0 全文继承+callSeq 重置切分；C2 自身段 282 行全是 glCreateProgram）
  ——本轮按 T547 纠正口径记录，v2 报告原文不改（原件留存）。
- CREATE_WAIT 非 owner 心跳；iter 计数非 wall 守护——不再据其断言 owner 状态。
- 旧 runner 的 exit124→业务 FAIL 翻译取消（watchdog 收据与业务断言分列）。

## NOT_RUN / 后续

- 机器表全量 sink（~70 函数）+14 高层 seam：本轮已覆盖 EGL/编译链 6+T488 wrapper 域，
  其余（reflection/resource/scene/FBO/高层 seam）按机器表逐组装配——交 developer 缺口
  seam（SOURCE_CREATE/RESOURCE_COMMIT/CAPTURE_EPOCH/SESSION_HANDOFF/READY 等）。
- C1_COMPUTE3/C2_ALL6 臂：待 seed 匹配方案（T547 §2 同 frozen 树+同 ANGLE DLL SHA+完整
  元组 seed-only 工厂）落地后与 C0 同链复跑。
- 业务 Create 卡点判定：维持 T530/T547 域（后段调用级证据待全量 sink）。

## 原件

layout_gate.py、gl_sink.inc（身份注入版）、run_create_only.sh、
gltrace-20261117-496v3{f,g}-c0.ndjson（含 v3f 的 JSON 缺陷行——留作修复证据）、
watchdog/child-pid 收据、out-*-stdout/stderr。
