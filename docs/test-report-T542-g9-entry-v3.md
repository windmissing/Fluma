# T542 v3 · G9 配置入口真机现场（T579-g9diag 包，T550 修复消费验证）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T579-g9diag-signed.hap（t562-g9-diagnostic，T550/T562 修复）。
证据：`test/reports/t491/t542b/`。

## 决定性进展：T546 G9 jscrash 缺陷真机闭合

操作链：冷启（bm clean -d）→ G9 独立研究页 → **K192 新运行**（触发
g9ResearchConfig typedarray 入口）→ 结果：

- UI 状态行：`G9_CONFIG_FAULT：LEGACY_NON_JSON_REPLY`——**不再 jscrash**
- **重复触发两次**（间隔 6s）：进程 PID 58829 持续存活、状态行稳定具名——
  T546/T542 原缺陷（SyntaxError 未捕获崩退）真机回归验证通过
- T550 UI 安全层（parseNativeObject）按 T546 §4 合同生效：裸 token 不再 JSON.parse
  崩溃，而是受控具名失败 `LEGACY_NON_JSON_REPLY`

## 新归因域（回 architect）

现场显示 native 侧 g9ResearchConfig 拒绝路径**仍返回裸 token**（UI 收到非 JSON 串）：
- 静态源码核对：app_bridge.cpp:1890 `g9ResearchConfig` 拒绝分支
  `return text(e,"G9_RESEARCH_CONFIG_REJECTED")`（裸 token）；T550 新增的
  `gpuInputDiagnosticsV1` 端点与 napi_words_contract.h 已在树内并已注册（app_bridge:1981）
- 但 g9ResearchConfig/m3 同类入口的**失败路径未改走 GPU_NAPI_ERROR_V1 JSON 包装**
  （T546 §3"普通失败也必须是对象 JSON"未在入口层生效）
- UI 侧无 attemptId 域（诊断端点未接 UI 消费）→ 无法从 UI 取 attempt 快照；
  files 目录无诊断冷原件（快照在 native 固定内存，需端点调用）

**归 T580 类同域**：修复矩阵中"JSON 端点安全错误"仅部分落地（新端点 ✓、旧入口
拒绝路径未包装 ✗）。T546 §5 判读分支 (a)（NAPI API 失败/NOT_EVALUATED→architect 核
runtime 契约）的补证前置仍缺 UI→attemptId 链。

## 对 T542 卡面域的推进

| 域 | 状态 |
|---|---|
| 包/设备 so/PID/boot 核 | ✅（冷启 PID 58829、bm clean 后首启） |
| 崩退回归（MUST-PASS） | ✅ PASS——重复触发无 jscrash，受控具名失败 |
| UI 发出数组 type/N/offset/预期词原件 | ❌ NOT_OBSERVED——UI 未暴露 attemptId/诊断消费 |
| native 同 attempt 诊断原 JSON | ❌ NOT_OBSERVED——同上，端点未接 UI |
| 根成本取证 | ❌ NOT_REACHED——配置入口仍拒，未进入根域 |

## 移交

- **architect**：T580 同域——旧入口拒绝路径的 JSON 包装缺口归因 + UI→attemptId
  诊断消费链缺口（T550 遗留）裁决
- **T545（M3 域）**：同包同构——M3 入口预期同样 LEGACY 具名化（下一轮验证）
