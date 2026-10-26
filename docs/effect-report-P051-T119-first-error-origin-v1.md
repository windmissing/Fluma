# P051/P011 T119 设备首错误原码补采效果报告 v1

日期：2026-10-26。责任：effector。任务卡：`6adf3c1ce4b017ab3404b7c6`。技术路线：`P051_TOUCH_TO_SOURCE_V1` 干场 DEMO。  
状态：**FIRST_ERROR_ORIGIN_CAPTURED / COLLECT99_FIRST_CAUSE_IDENTIFIED / NOT_A_PRODUCT_PASS**。  
T117 首错误诊断在设备上完整工作；**COLLECT99 的首因被原码级定位**：`PARTICLE_BEGIN → BEGIN_HOT → FIELD_COLUMN 谓词失败（Hot rawCode=2 INVALID_INPUT）`，即 **预览 COLLECT 的 beginStepHot 在现场 field 某列读到非有限/负值**。

## 0. 环境与执行

| 项 | 值 |
|---|---|
| commit / 树指纹 | `2c024c8`（T117 诊断）/ `cd9401cd54661870` |
| HAP | `entry-default-s147-signed.hap`，SHA256 `707d72135c2544ffe57af80c6f01e685fb3c5c3057f53026d4342d692196d0eb` |
| 设备 / 窗口 | VYG-AL00 `62T0225B10010020`；22:47:09–22:48:44 |
| 执行 | `tools/t119_first_error_capture.ps1`（C1/C2 两臂，T103 同款 4×swipe 序列） |
| 产物 | `test/artifacts/t119-first-error-capture-20261026/`（两臂 48 片 trace 全捕获、截图、dump） |

## 1. 首错误原码（两臂四条 trace 一致）

两 run 均复现同构序列：trace0（前一子步）`PLANS_READY` 提交成功（C1 commit 574→575 / C2 442→443），**紧接着的下一子步** trace1 在 theta=42 REQUESTED 候选的 COLLECT 阶段失败 → blocked → code5。

| 字段 | C1（substep 566） | C2（substep 438） |
|---|---|---|
| `firstFailure.origin` | **PARTICLE_BEGIN** | **PARTICLE_BEGIN** |
| `phase` | **BEGIN_HOT** | BEGIN_HOT |
| `rawCode`（Hot 原码） | **2 = P011_HOT_INVALID_INPUT** | 2 |
| `m2Code` | 3 = STEP_FAILED | 3 |
| `predicate` | **FIELD_COLUMN** | **FIELD_COLUMN** |
| `failureIndex` | **列 67** | **列 46**（跨 run 不同列） |
| entry/exit 状态 | stepOpen=0, active=0, pageBacklog=0, budgetOpen=0（**无 BUSY/无 backlog**） | 同左 |
| rollbackCode | 0 | 0 |
| drift 三分项 | 全 0（limit 0.001）| 全 0 |

## 2. 首因判定（T118 三类99 分类法的设备落定）

按 T118 的三类99 分类：本卡设备证据落在**类别 2 变体——BEGIN_HOT 原码折叠**，但谓词不是 V2 构造的 `PAGE_BACKLOG`，而是 **`FIELD_COLUMN`**（`beginStepHot` 逐列校验，`P011ParticleHot.ets:519`）：现场 field 某列出现**非有限值或负分量**（waterTop/bh/bl/mh/ml/suspH/suspL/surface 之一）。

**关键排除**：
- **非 BUSY 重入**（T103 报告的首要嫌疑）：entryStepOpen=0、entryActive=0、pageBacklog=0——事务入口干净，重入假设被否定；
- **非 AUDIT drift**（V3 类）：drift 三分项全 0；
- **非 S0 本身非法**：trace 自带 s0 快照全 0、无 NaN，且失败列在 s0 中为 0——**非法值是"失败时刻的现场 field"与"trace s0 快照"之间的差异**。

**剩余根因方向（唯一）**：成功提交的前一子步（C1 substep 565 / C2 substep 437，commit +1）的**写场路径**（粒子 apply/LAND 写列、或 dry 通道）把某列写成非法值；该污染在下一子步的预览 COLLECT `beginStepHot` 现场校验时才被捕获。失败列不固定（67/46）支持"运行时写场污染"而非固定边界。T109 §3"poolProgress 摘要层不排除粒子/预算分支失败"的保留判断被证实。

## 3. 判定与移交

| 项 | 判定 |
|---|---|
| T117 诊断设备验证 | **PASS**（origin/rawCode/phase/predicate/failureIndex/entry-exit 状态全链到设备，两 run 可复现） |
| COLLECT99 首因 | **PARTICLE_BEGIN/BEGIN_HOT/FIELD_COLUMN（Hot INVALID_INPUT=2）**——前一成功子步写场污染列值 |
| BUSY 重入假设 | **排除**（entry 全零干净） |
| T103 报告勘误点 | T103 §3"首要嫌疑=预览与真实步重入冲突"被设备证据否定，以本卡为准 |
| 移交 | **developer/architect**：审计前一子步（成功 commit）的粒子 apply/LAND 写列路径，找产生非有限/负列值的通道；trace 内 `authoritativeBefore/AfterDigest` 与 `particleEventTail` 可关联失败子步的具体写入 |
| T95 | 继续阻塞（等修复） |

## 4. 边界声明

- 产品代码零修改；s147 隔离构建。
- trace 原文（两臂各 48 片 COMPLETE dropped=0）已入库；分片重组协议与 T103 相同（外层信封 part/parts/text → 内层 payload）。
- 滴答 T119 卡评论已发；清单行状态留 manager。
