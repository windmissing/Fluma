# T545 v3 · M3 V4 四档真实设备期限补证（T579-m3diag 包，T550 修复消费验证）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T579-m3diag-signed.hap（t562-m3-diagnostic，T550/T562 修复）。
证据：`test/reports/t491/t545b/`。

## 现场（M3 域，T550 UI 安全层生效验证）

操作链：冷启 → M3 V4 页 → **新建运行**（触发 N16 typedarray 配置入口）→ 结果：

- UI 状态行：`M3_CONFIG_REJECTED（可读诊断：native.gpuInputDiagnosticsV1('M3A')）`
  ——**不再崩退，且 UI 直接给出诊断端点消费指引**（M3 域 UI 已接诊断指引；G9 域无
  此指引，域差异确认）
- **重复触发两次**：进程 PID 61917 持续存活、状态行稳定——T545 原 M3 配置崩退域
  真机回归验证通过（T550 UI parseNativeObject 具名化生效）

## 与 T542 v3 同构缺口（归 T580 域）

- M3 入口拒绝路径 native 侧仍裸 token（UI 显示具名 REJECTED 而非 JSON 错误对象
  展开）→ `gpuInputDiagnosticsV1('M3A')` 需 UI 显式调用，当前页仅展示指引文本、
  未自动消费 → attempt 快照（lastAttempt/firstFailure）未落盘到可取回原件
- files 目录无诊断快照冷原件（快照在 native 固定内存）

## 对 T545 卡面域（四档真实设备期限补证）的推进

| 域 | 状态 |
|---|---|
| 包/设备/PID/boot 核 | ✅（PID 61917、bm clean 后首启） |
| 崩退回归（MUST-PASS） | ✅ PASS——重复触发无 jscrash，具名失败 |
| 四档期限取证（K192/512 全档真机 GPU 域） | ❌ NOT_REACHED——M3 配置入口仍拒（typedarray 解码链被拒），未进入期限域 |
| NAPI 逐 API 观测（T546 §5 判读分支 a/b/c） | ❌ NOT_OBSERVED——诊断端点未接 UI 自动消费 |

## 移交

- **architect（T580 同域）**：①入口拒绝路径 JSON 包装缺口（新旧实现绑定表归属：
  app_bridge.cpp 旧实现仍在 GRS_APP_M1 绑定表，napi.inc 新实现未接入）；②UI→
  attemptId→gpuInputDiagnosticsV1 消费链（M3 已有指引文本、缺执行；G9 完全缺失）
- 该两缺口修复后，T545 四档期限取证与 T542 根成本域可立即续测（包已就绪）
