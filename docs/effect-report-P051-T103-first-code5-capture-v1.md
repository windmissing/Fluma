# P051/P011 T103 DEMO 首个 code5 设备补采效果报告 v1

日期：2026-10-26。责任：effector。任务卡：`6adf27a0e4b06ee35742e945`。技术路线：`P051_TOUCH_TO_SOURCE_V1` 干场 DEMO（T82 同页）。  
状态：**DEVICE_TRACE_CAPTURED / FIRST_CODE5_MECHANISM_LOCALIZED / COMPLETE_NO_DROP / NOT_A_PRODUCT_PASS**。  
T102 有界导出机制在设备上完整工作；首个真机 code5 的 planner 阻断机制被精确定位到 **预览 COLLECT 阶段粒子事务返回非 0 → UNKNOWN(99)**，交 architect（T104/T90 线）。

## 0. 环境与执行

| 项 | 值 |
|---|---|
| commit / 树指纹 | `c0c58c3`（T96 池优化 + T102 诊断）/ `926b87fce4e7842b` |
| HAP | `entry-default-s145-signed.hap`，SHA256 `553507fcbe435d6a266ce87ab27e09fefb7d5c2c065c18e97226419c337e83a7` |
| 设备 / 窗口 | VYG-AL00 `62T0225B10010020`；19:04:59–19:06:38 |
| 执行 | `tools/t103_first_code5_capture.ps1`（C1 复现 / C2 重布防验证）；产物 `test/artifacts/t103-first-code5-capture-20261026/` |

## 1. 捕获结果（T102 导出机制设备验证）

| 臂 | trace 分片 | 重组 | status |
|---|---|---|---|
| C1 | 3 片（part 1–3，`complete=true`） | payload 97KB 级 | **COMPLETE，dropped=0** |
| C2 | 48 片 | **两条 trace**（双 run 均触发） | **COMPLETE，dropped=0** |

分片协议（`P051_FIRST_CODE5_TRACE_V1`，part/parts/text）在 hilog ~4KB 行限下完整重组成功；两次冷启新 run 均重新布防并成功导出（用户 reset 重布防语义验证）。**无 TRACE_INCOMPLETE、无丢片。**

## 2. 首个 code5 的机制证据（C1 payload 全字段）

失败时刻身份：run `p011-run-1793012705902-3`、attempt `…-a675`、substepId 675、beforeCommit=afterCommit=682（**失败步未提交**，回滚语义正确）、medium=DRY_AIR_V1、dt=1/60。

| 字段 | 值 | 含义 |
|---|---|---|
| effectiveTiltDeg | 30.0（target 79.75 被 T50 钳制） | 设备平放姿态 |
| sourceEpoch / externalAdmissionCount | 6 / 6 | 6 笔手势注入已准入 |
| pool | live=36, **airborne=36, contact=0** | 粒子全部悬空 |
| poolProgress | seedCalls=6, collectCalls=4042, **mergeCalls=0, firstError=0, rollbackCount=0** | 池侧无错误、无 MERGE 参与 |
| trace.resultStatus | **blocked**，blockedReason=**NO_SAFE_OLD_TILT_PLAN** | 规划器拒绝 |
| trace.candidates[0] | theta=42, stage=**COLLECT, collectCode=99**, collectColumn=-1 | **唯一候选在预览 COLLECT 阶段失败** |
| trace.notExecuted | `['CLAMPED:REQUESTED_blocked', 'OLD:REQUESTED_blocked']` | 回退候选被连坐未执行 |
| trace.s0 / preexistingResiduals | 128 列快照；127 残差全 -0.5773，**preexistingViolation=false** | S0 合法（非既有违例触发） |
| trace.hardMax | 0.5774（tan30） | 安息角合同在位 |

## 3. 机制定位（对照 C2 trace0 的关键差异）

- **C2 trace0（同 run 较早时刻，substep≈674 前后）**：候选 theta=42 与 theta=30 **双双 `PLANS_READY`**（规划可成）——notExecuted 仅 `OLD:PRIOR_CANDIDATE_clamped`（选优拒绝，非阻断）。
- **C1/C2 trace1（粒子 36 颗全部 airborne 时）**：theta=42 候选在 **COLLECT 阶段 `collectCode=99`**（`WaterSandErrorCode.UNKNOWN`），theta=30 回退候选被 `REQUESTED_blocked` 连坐 → 整体 blocked → code5。

**代码路径**（T103 走读）：`previewChannelPlansAt(theta)` → `substep(dt)` → `beginCollect()`（`P011ParticleM2Field.ets:171`）。`beginCollect` 非 0 返回 `P011_PARTICLE_M2_BUSY=1`（stepOpen≠0，:173）或 `P011_PARTICLE_M2_STEP_FAILED=3`（beginStepHot/collect/compare 失败，:175–182）→ `WaterSandField.ets:4321` 置 `lastError=UNKNOWN(99)` → COLLECT stage 记 99。

**根因方向（供 architect 验证）**：预览 COLLECT 在 theta=42 且池内 36 粒全 airborne 时，粒子事务 `beginCollect` 返回 BUSY/STEP_FAILED——首要嫌疑是**预览 substep 与真实 step 的粒子事务重入冲突**（预览在 planning 中调 beginCollect 时，真实步的 stepOpen 未清或被预览自身前一次候选打开）；次要嫌疑是 beginStepHot 内部（双计划 collect/compare）在 airborne 集合下的确定性失败。池侧与高度场 S0 均无错误（mergeCalls=0、firstError=0、residual 全合法），**排除容量/安息角/审计 drift 路径**。

**T78 主线程卡死的关联澄清**：本卡池诊断显示 T96 优化后 `seedApplyMs=23ms`、`mergeCalls=0`——T82 触发的 code5 与 T78 触发的 THREAD_BLOCK 是**两个不同故障**（本卡 12s 无卡死）；T78 卡死修复仍按 T92/T96/T97 线推进，不因本卡结论改判。

## 4. 判定与移交

| 项 | 判定 |
|---|---|
| T102 导出机制设备验证 | **PASS**（两 run 完整导出、分片重组、重布防语义） |
| 首个 code5 机制 | **定位**：预览 COLLECT 粒子事务非 0 → UNKNOWN(99) → NO_SAFE_OLD_TILT_PLAN（S0 合法、池无错、排除容量/drift） |
| code5 记 invalid | 是（fail-closed，不计主观分） |
| 移交 | **architect（T104/T90 线）**：按 §3 根因方向做最小修复设计；不放宽 guard，不盲改规划器 |
| T95（LAND 长窗复测） | 继续阻塞（等 architect 裁定及必要修复），不能因诊断可采提前放行 |

## 5. 边界声明

- 产品代码零修改；s145 隔离构建（worktree）。
- 本报告为机制取证，不是产品 PASS；code5 本身仍是待修缺陷。
- trace 原文（97KB C2 / C1 全文）与两臂 dump 已入库 `test/artifacts/t103-first-code5-capture-20261026/`。
- 卡评论已发滴答 T103 卡。
