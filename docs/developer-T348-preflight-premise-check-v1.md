# T348 · 触摸倒沙接线移植——开工前置勘察报告（前提与代码现状冲突，暂停实施交 manager 裁定）

日期：2026-10-31。责任：developer（鸿蒙开发工程师）。任务卡：滴答 `6ae5e40fe4b031ea8306dd6b`。
状态：**PREMISE_CONFLICT / NOT_IMPLEMENTED / ROUTE_TO_MANAGER_DECISION**。

## 0. 结论

任务卡的开工前提（「EmptySandDemo 触摸只调 model.start/move 记录诊断坐标，『触摸→生成沙粒』链路从未接线」）**与代码现状冲突**。按 developer 工作流「发现文档与现状冲突时停下来反馈，不擅自选择」，本卡暂停实施，出具证据交 manager 裁定。**未改任何产品代码。**

## 1. 冲突证据（三路独立核实，全部可复现）

### F1 源码证据：触摸→沙链自 T71 起就存在

`P051EmptySandDemo.tick()`（entry/src/main/ets/physics/P051EmptySandDemo.ets）touching 路径：`clock.sample(frameTimeNs,'ns')` → 构造 `P051DryAdmissionRequest{revision:P051_DRY_REVISION, sourceId:51, sequence:++this.sequence, x:this.x, y:this.y, seed:51, requestedH:8*seconds}` → `field.injectDryGestureSand(request)`。git 考古：`0b1f7e3 feat(p051): connect touch input to dry source admission (T71)` 即该接线的落地 commit。页面链完整：`EmptySandDemo.touch()` → `touchSource.handle()` → `model.start(x,y)/move`（128×256 坐标映射）→ 每帧 `tick()` 注入。

### F2 运行时证据（真 ArkTS Node harness，沿 T346 机制）

test/t348_preflight_premise_check.cjs（4/4 PASS，证据 test/reports/t348/preflight-premise-check.json）：

| 用例 | 结果 |
|---|---|
| T348-P1 触摸 1s | start ACCEPTED，60 tick 后 sourceEpoch>0（注入事务提交） |
| T348-P2 触摸 6s | liveCount=512（池满），injectedH=34.13 units |
| T348-P3 续持 700 步 | pYmax 220→0.58 下降，**mh 床增长 0.5774（LAND 落地）**——全链 stage→admission→born→沉降→LAND→床增长 就是本卡要求的验收断言，现状已满足 |
| T348-P4 reset 空场 | baselineStatus=EMPTY_BASELINE，water/heavy/light 全 0 |

### F3 介质证据：按卡面移植 stageGestureSandAndFeed 会产生 NO_WATER

EmptySandDemo 场是 `DRY_AIR_V1`（reset → initializeDryParticleDemo）。卡面参照的 SandCanvas `stageGestureSandAndFeed` 走 `field.injectGestureSand`（湿链，per-column 三角核 8 unit/s×dt），湿链在干场返回 `NO_WATER`（WaterSandField.ets:1849-1851 anyWet 判据）。**干场的正确触摸注入链就是现状的 `injectDryGestureSand`（点注入，8×seconds，同一速率设计值）**——两链介质互斥，卡面「照 SandCanvas 先例移植」在 EmptySandDemo 上不可行。

### F4 空场场景已存在

「重置为空场」按钮 → `resetDemo()` → `model.reset()` → `initializeDryParticleDemo` → `initialize({waterRegions:[],heavyDepositedRegions:[],lightDepositedRegions:[],heavySuspendedRegions:[],lightSuspendedRegions:[]})`——全空 regions 即卡面要求的空场场景，无需新增选项。

### F5 设备实证（今日 demo 录像）

commit `c5ad113`（20:07）与 `42ac464`（20:21，docs/visual/v1_full_interaction.mp4）已录制 V1 EmptySandDemo XComponent 页「空场重置→触摸倒沙（粒子下落）→床堆积→倾角沙丘形变」全程——触摸倒沙在真机上已工作并有影像证据。T343/T347 走 T218DBG 注入是 V2 页（P051DerivedV2Demo）的采集手段，非「绕过缺口」。

## 2. 可能的解释（供 manager 核对，不代裁）

- 卡面所指可能是**特定 Want 参数形态**或**另一页面入口**的缺口——建议 manager 复核 20:40 发现的具体复现路径（页面、参数、现象）；
- 或卡面意图是 V2 页（P051DerivedV2Demo）补触摸注入——但该页 `touch()` 已接 `model.start/move` → `P051DerivedDemoV2.tick` → `owner.inject`（同 8×seconds 速率），T347 真机 23 笔注入即走此链；
- 或意图是 SandCanvas 主页面（湿场景）→ EmptySandDemo（干场景）的**场景迁移**——那是需求扩展不是「接线缺口」，需 designer/architect 先出合同。

## 3. 红线与现状

- 未改任何产品代码；未改物理/守恒/renderer；T216 七套件复跑 7/7（无回归基线确认）；
- 本报告+preflight harness 是唯一产出（测试资产，非产品改动）；
- 不宣布卡完成（实施未做）；不自行撤卡——交 manager 裁定后按新口径续做或撤卡。

## 4. 变更记录

- 2026-10-31 v1：初版。五路证据（源码/运行时/介质/场景/设备）证实卡面前提与现状冲突；暂停实施；路由 manager 裁定。
