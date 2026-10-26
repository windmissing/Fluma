# T70 触摸零源客观补证 v1

日期：2026-10-26。责任：tester。任务：[6adeac6e…4d36](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6adeac6ee4b03273ad764d36)。状态：**EVIDENCE_ONLY / NOT_A_FIX / ATTRIBUTION_BELONGS_TO_T69 / RED_BASELINE_FOR_T71 / DEVICE_FACTS_PARTIALLY_ABSENT**。

不使用真机；不改业务、判据、物理与测试判线。消费 T63 固定版本（快照 `86366dd`）与原始日志，真实模型离线夹具逐级追踪 触摸事件 → 源请求 → 准入 链，区分"事件未达"与"物理拒绝"，为 T71 提供修复前 RED 基线。总体归因归 T69。

## 0. 证据文件与模型身份

| 文件 | 内容 |
|---|---|
| `test/p070_touch_zero_source_units.cjs` + `test/reports/p070_touch_zero_source_units.json` | 真实模型夹具 7/7 PASS（正例 + 机制 + 设备日志交叉核对） |
| `test/p070_t63_snapshot_model_check.cjs` + `test/reports/p070_t63_snapshot_model_check.json` | 关键三例在 **86366dd 精确字节**（T63 构建）上复跑，3/3 PASS |
| `test/reports/p070_t63_archive_phase_analysis.json` | T63 五阶段日志逐阶段事件统计 |
| 模型身份 | 工作树的 `P051EmptySandDemo.ets`/`P051GravityInputAdapter.ets`/`EmptySandDemo.ets` 与 86366dd **有差异**（T58/T59 干场工作，不在 T63 构建内）；关键三例已改用 git 快照字节复跑，结论对 T63 构建成立 |

## 1. T63 归档逐级证据（事件层 → 源层 → 准入层）

### 1.1 事件层（设备缺口的第一现场）

- **T63 页面没有记录任何一条触摸事件**：五个阶段日志 grep `touch/TouchType` 均为 0 匹配；页面只有 60 帧一次的 `empty_sand_demo` 抽样（含重力输入对象）与每秒一次的 `p013_dual_frame/geometry` 回执。
- 五阶段共 106 个抽样，`input.source` 分布 **全部为 sensor**（无一个 stale 抽样）；全部 106 个抽样 `sourceInjectedHeavy/particleMass/depositedMass = 0`。
- 无 `p013_dual_rejected`、无 `gesture_sand`/`NO_WATER` 准入回执（demo 模型未安装 `setGestureSandEventSink`，事件回执通道在 DEMO 页面不存在）。

**结论：事件层在设备上不可观测——无法区分 (a) onTouch 未收到事件、(b) 收到但坐标换算/model.start 未生效、(c) model.started 后被 stale 停触。这是 T69 归因的首要缺口。**

### 1.2 源层与准入层（离线可判定部分）

真实模型夹具（T63 精确字节）逐级结果：

| 用例 | 场景 | 结果 | 判定 |
|---|---|---|---|
| T70-A01 | touching=true、直接模型坐标、2s tick | sourceInjectedHeavy>0、particleMass>0、deposited>0，首个非零在 60 帧内 | **物理准入层无阻断**：合法触摸的源请求被正常准入 |
| T70-B01/B02 | 重力适配器 20ms 采样 vs 16.7ms tick | 250ms 内 fresh；>250ms → source=stale、targetTiltDeg=null、valid=true | 契约确认 |
| T70-B03 | 单次 stale 输入 | **一次 stale 即 model.stop()，触摸终止于任何注入之前** | 零源机制候选①复现 |
| T70-C01 | touching=true、无任何重力输入 | 照常注入（input===null 从不 stop） | "传感器从未投递"单独不构成零源——适配器首样后永不返回 null，走的是 stale 分支 |
| T70-D01 | 每 tick 喂 stale ×2s | source/particles/deposited 全零 | **设备零源签名在离线可复现**（候选①全量复现） |
| T70-E01 | T63 设备抽样交叉核对 | 全部 source=sensor、带 fresh targetTilt（静置角 ~77.6°） | **stale 停触机制未获设备证据支持**（抽样不穷尽，边界 0–250ms 窗口不可见） |

### 1.3 分流意义（供 T69）

- **物理拒绝被排除为唯一根因**：A01（快照字节）证明准入层对合法触摸源正常放行；且 T63 设备日志无任何准入回执——源请求根本未产生或未到达。
- **候选机制 A（stale 停触）**：离线可完全复现零源签名（D01），但设备抽样全部 fresh sensor——若成立，stale 只能发生在抽样间隙（60 帧 1 次，0–250ms 边界不可见）。需要逐 tick 的 input.source 记录才能证实/排除。
- **候选机制 B（事件未达页面）**：页面无触摸日志，onTouch 是否被 XComponent/Stack 分发完全不可见。注意 T63 版本 Stack **没有** `hitTestBehavior`，XComponent 为 TEXTURE 型——事件分发行为属设备可观测项。
- **候选机制 C（坐标换算/model.start 未生效）**：换算式 `point.x/surfaceWidth*128` 中 surfaceWidth 来自 onAreaChange（T63 已改为 XComponent 自身），无日志可比对。
- 三机制在现有日志下**互不排斥且不可判**；T69 需按 §3 缺字段清单设计设备补采。

## 2. 责任边界与最小复现

- **最小复现（行为级，沿 T63）**：T62 快照包 → 冷启 → 新管线成功 → 容器内自动/手动触摸 → 零源/零粒子，native particleCount=0，106/106 抽样零源。本稿补充：该行为与"每 tick stale"离线签名一致，但与全部 fresh 抽样并存，未闭合。
- **tester 边界**：本稿只确定"哪些环节被排除、哪些不可判"，不断言设备根因；T69 负责分流，T71 修复。
- **T71 RED 基线**：A01 即修复后的最小正例判据（真实模型 + 触摸坐标 → 非零源），修复验证时按同路径复跑；D01 为 stale 场景的保持用例（修复不得破坏 stale 停触语义）。

## 3. 设备缺字段清单（补采需求，交 T69/manager）

1. **页面触摸事件日志**：onTouch 收到的每条事件（type、local x/y、timestamp、touches.length）；
2. **model 生命周期日志**：start/move/stop 的调用时刻与坐标参数；
3. **逐 tick input.source 记录**（含 stale 命中计数），覆盖 0–250ms 边界；
4. **sensor 投递计数器**：sensor.on 注册后实际收到的样本数与时间戳（区分"注册失败/无投递"与"投递正常"）；
5. 准入回执通道：DEMO 模型接入 `setGestureSandEventSink`（现仅在 SandCanvas 安装），使 NO_WATER/准入量可见。

## 4. 不涉及项

- 不改业务/判线/物理；不宣称 T63 任一 FAIL 已修复；不把离线正例当设备通过；
- 不使用真机；用户"仅新管线、禁止 Canvas 回退"约束已知悉（本卡未触碰渲染路径）；
- 干场准入（T58/T59）、上传器裁定（T67）、纯新管线合同（T68）不在本稿范围。

## 5. 变更记录

- 2026-10-26，v1：首次发布。事件层不可观测（0 触摸日志）、源/准入层离线判定（A01 正例排除物理准入、B03/D01 stale 停触签名复现但 E01 设备抽样不支持）、三候选机制互斥性不可判、五项设备缺字段交 T69。
