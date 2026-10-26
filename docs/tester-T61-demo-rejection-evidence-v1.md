# T61 DEMO 拒绘客观补证 v1

日期：2026-10-26。责任：tester。任务：[6ade982c…3dd71](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6ade982ce4b03273ad73dd71)。状态：**EVIDENCE_ONLY / NOT_A_FIX / NOT_A_PASS_OF_THE_PRODUCT / DEVICE_FACTS_ABSENT**。

不使用真机；不改业务代码、不改冻结判据；整体归因归 T60（已交付），本稿只按 T60 §5 需求补 T52 F1/F2 的离线客观证据。执行器 `test/p061_demo_rejection_evidence.cjs`，证据 `test/reports/p061_demo_rejection_evidence.json`：**25 例 25 PASS、0 FAIL、0 ERROR**。

## 0. 证据身份

| 项 | 值 |
|---|---|
| 被测源码 | 真实生产文件（非复制品、非改写）：`P013RenderController.ets`、`P013DualFieldAdapter.ets`、`P051EmptySandDemo.ets`、`P011ParticleHot.ets`、`P011ParticleField.ets`、`napi_bridge.cpp` |
| 与 T52 构建一致性 | 六个文件 SHA-256 与 `test/artifacts/t52-empty-sand-demo-20261025/source-sha256.txt` 及当前工作树逐项一致（6/6 MATCH；执行器 JSON `sourceIdentity`） |
| native 通道 | Node Module._load 桩替换 `libp013renderer.so`（与既有 `run_p013_composition_units.cjs` 同一约定）；controller 的 `submitDualField`/adapter 的 `packInto` 全部为生产代码原路径 |
| 合成声明 | 所有 native 尺寸均为桩值并在每例标注 `synthetic`；**任何记录都未把 1280×2832 屏幕尺寸当作 XComponent surface 尺寸** |

## 1. F1：尺寸互锁（真实 controller 逐帧证据）

### T61-F1-A01 启动互锁复现（T60 §3.1 确定坏路径的实测版）

前置：native 桩返回 pre-surface 状态（widthPx=0/heightPx=0、sizeSource=UNAVAILABLE），页面逻辑 Area 为带小数的合成值（1177.6×2442.8，标注 synthetic；T52 设备实际逻辑宽高为缺失事实）。逐帧 5 帧结果：

- 每帧 `submitDualField` 返回 null，`dualFailure='NONUNIFORM_SCALE'`；
- **native drawDualFieldFrame 调用数恒为 0**（5/5 帧）——拒绘发生在 native draw 之前，EGL surface 创建从未发生；
- 每帧 getState 调用数单调递增（帧 0 两次、此后每帧一次）——`GetState` 持续查询但 surface 仍不存在，widthPx 恒 0；
- `lastPresentedCommit` 恒 0，无一帧提交。

结论：T60 §3.1 的启动互锁在真实 controller 上逐帧复现——fallback 用两轴独立取整的逻辑量算 sx/sy，`|sx−sy|>1e-6` 在 native draw 前 return；下一帧仍 widthPx=0，重复拒绘。这是代码路径证据，不冒充 T52 设备实测宽高复原。

### 其余 F1 用例（护住 T60 §3.3 修复方案的边界）

| 用例 | 场景 | 结果 |
|---|---|---|
| T61-F1-B01 | 桩报真实 surface（800×480）、逻辑同值整数 → 过门、到达 native draw、swap 成功、lastPresentedCommit 更新 | PASS |
| T61-F1-C01 | 真正非均匀（逻辑 800×300 vs surface 800×480）→ NONUNIFORM_SCALE 保持、draw 未调 | PASS |
| T61-F1-D01 | 小数逻辑轴 vs 整数 surface（799.6×480.4 vs 800×480）：sx=1.0005003…、sy=0.9991674…、比值差 1.33e-3 ≫1e-6 → 拒绝、draw 未调 | PASS |
| T61-F1-E01 | 外层带边框 Stack 测量（804×484）vs XComponent 内容区（800×480）→ 拒绝 | PASS |
| T61-F1-E02 | 同节点内容区测量（800×480）→ 过门、到达 draw | PASS |
| T61-F1-F01 | 互锁只在真实 surface 尺寸出现后解除：帧 1（0×0）拒，帧 2（桩改报 720×480）即过门到达 draw | PASS |

## 2. F2：INVALID_PARTICLE 逐谓词分解（真实 adapter）

### T61-F2-A01 合法源 Float32 量化反例（T60 §4.1 证实）

对真实 field+pool 发布（128 列、水量 160、粒子路由激活、注入后 live=1、slot0）逐一设置合法 Float64 右边界源坐标：

| x64（Float64 合法 <128） | fround 后 | adapter 结果 |
|---|---|---|
| 127.99999999 | 128 | INVALID_PARTICLE |
| 127.999999999 | 128 | INVALID_PARTICLE |
| 127.9999999999 | 128 | INVALID_PARTICLE |
| 127.99999618530273（中点） | 128 | INVALID_PARTICLE |

中点=127.99999618530273（127.99999237060547 与 128 的中点）起即舍入到 128——与 T60 §4.1 静态推导逐值一致，现在有真实 adapter 的执行证据。**这是 T62 启用 `COLUMN_PRESERVING_F32_RENDER_V1` 投影分支的冻结前提**（T60 §4.3 的"仅在 T61 证实后启用"条件已满足）。

### T61-F2-B01 Y 域边界

Y 闭域 `[0,512]`（本夹具 capacity=512；DEMO 场为 256，均为可精确表示的 Float32 整数）：y=512 与 511.9999999999999 均合法（OK）；y>512（600）拒绝。Y 方向不存在 X 方向的开右端点量化缺口。

### T61-F2-C01..C14 全 OR 分支（首个被拒 slot 与字段全记录）

每例记录 actualCode、firstRejectedSlot、该 slot 的 pId/state/phase/mass/x64/y64/xF32/yF32/massF32。全部按期望拒因拒绝：

| 用例 | 谓词 | 拒因 |
|---|---|---|
| C01/C02/C03/C04 | state∉{1,2} / phase>1 / mass=NaN / mass≤0 | INVALID_PARTICLE |
| C05/C06/C07/C08/C09 | x=NaN / x<0 / x=128.5 / y<0 / y>capacity | INVALID_PARTICLE |
| C10/C11 | pId=0 / pId 超出 uint32 | INVALID_PARTICLE |
| C12 | 活粒重复 ID | DUPLICATE_ID |
| C13 | liveCount 不一致 | CAPACITY_MISMATCH |
| C14 | 发布 token 失配（field.physicsCommitId 递增） | PUBLICATION_MISMATCH |

### T61-F2-D01 包原子性

有效包 → 注入 127.99999999 → INVALID_PARTICLE → **前一有效包 particleMeta/particleXY 摘要逐字节不变**、输入发布逐字节不变 → 恢复源后重打包与首个有效包逐字节一致。T60 §6"拒绝不改 publication、不覆盖上一有效 packet"在真实 adapter 上成立。

### T61-F2-E01 动态可达性观测（观测项，非判据）

真实 P051EmptySandDemo 模型右向移动 400 步（0.5→127.999 线性移动、每步注入+step）：本夹具下最大 x64=127.99500493（fround=127.99500274，**未达量化边界**）。该结果只说明此轨迹参数未触发边界；不外推为"设备上不可能触发"，也不构成任何判据。

## 3. 缺失事实清单（设备独有，禁止用夹具冒充）

| 缺失项 | 为什么夹具补不出 | 处置 |
|---|---|---|
| T52 当时的真实逻辑 viewport / surface 尺寸 / DPI / sx、sy 实测值 | T52 三份 hilog 无 `p013_dual_frame` 回执与逐帧尺寸；`controls.json` 是控制中心 UI 树，不含本 DEMO XComponent 节点 | 若 manager 需要，另派带真机标签的采集卡；否则按 T60 §3.1 代码路径修复（本稿 A01 已复现该路径） |
| T52 首个被拒 slot 及其 pId/state/phase/mass/x64/y64 | T52 未保存逐粒快照 | 同上；T63 修复后复测时按本稿 F2 输出格式补采 |
| 设备同一 F1/F2 修复后是否消失 | 需真机 | T63（effector，等 T62） |

## 4. 交 T60/T62 的结论

1. **F1**：启动互锁代码路径在真实 controller 上复现（A01）；同时确认均匀通过（B01/E02）、真正非均匀保护（C01）、节点错位敏感（E01）、小数取整差异（D01）与互锁解除条件（F01）。T62 可按 T60 §3.3（prepare 真实 surface + 显式像素几何 + 同节点测量）实施，无需放宽 1e-6 或引入 epsilon。
2. **F2**：全部 OR 分支均有独立反例与首个 slot 证据；合法右边界量化反例（A01）已在真实 adapter 上冻结，T62 投影分支的启用前提满足。C01–C11/C13 的反例同时保证 T62 修复不得把"原始非法源"夹回域内——非法源仍须整帧拒绝。
3. **不变量核验**：拒绝前后包与发布逐字节不变（D01 + 各例 packetUnchanged/publicationUnchanged 标志全真）。
4. **缺口**：§3 三个设备独有事实缺失；本稿不做设备根因断言，T52 设备上 F2 的具体触发分支保持 T60 §1 的"未证实"状态。

## 5. 不涉及项

- 不修业务代码；不改判据/阈值；不新建错误码或容差；
- 夹具中的 native 桩与合成逻辑尺寸均已标注，不冒充设备证据；
- 干场准入（T58/T59）、T47 规划阻断补证不在本稿范围；
- 不使用真机；产品状态仍为 FAIL_RENDER_GATE，本稿不改变该状态。

## 6. 变更记录

- 2026-10-26，v1：首次发布。25/25 PASS；T52 源码身份 6/6 一致；F1 启动互锁与六项边界复现、F2 十四谓词+量化反例+包原子性+轨迹观测；三项设备独有事实列为缺失交 manager。执行器 `test/p061_demo_rejection_evidence.cjs`；证据 `test/reports/p061_demo_rejection_evidence.json`。
